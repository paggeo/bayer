#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "sleep.h"
#include "xaxidma.h"
#include "xtime_l.h"

#define TX_DMA_ID                 XPAR_PS2PL_DMA_DEVICE_ID
#define TX_DMA_MM2S_LENGTH_ADDR  (XPAR_PS2PL_DMA_BASEADDR + 0x28) // Reports actual number of bytes transferred from PS->PL (use Xil_In32 for report)

#define RX_DMA_ID                 XPAR_PL2PS_DMA_DEVICE_ID
#define RX_DMA_S2MM_LENGTH_ADDR  (XPAR_PL2PS_DMA_BASEADDR + 0x58) // Reports actual number of bytes transferred from PL->PS (use Xil_In32 for report)

#define TX_BUFFER (XPAR_DDR_MEM_BASEADDR + 0x10000000) // 0 + 512MByte
#define RX_BUFFER (XPAR_DDR_MEM_BASEADDR + 0x18000000) // 0 + 768MByte

#define TX_BUFFER_BASE		(XPAR_DDR_MEM_BASEADDR + 0x00100000)
#define RX_BUFFER_BASE		(XPAR_DDR_MEM_BASEADDR + 0x00300000)

#define N 4

#define Max_packet_len N*N*100

int A[N][N];

int G[N][N];
int B[N][N];
int R[N][N];

/* User application global variables & defines */

XAxiDma AxiDma_TX;
XAxiDma AxiDma_RX;

u32 *Rxtest;

void sw_fix(){
	int i,j;
	int UL,UR,UC,LR,LL,LC,CR,CC,CL;
	for (i = 0; i < N; i++)
		  {
		    for (j = 0; j < N; j++)
		    {

		      if (i == 0 || j == 0)
		      {
		        UL = 0;
		      }
		      else
		      {
		        UL = A[i - 1][j - 1];
		      }

		      if (i == 0)
		      {
		        UC = 0;
		      }
		      else
		      {
		        UC = A[i - 1][j];
		      }

		      if (i == 0 || j == N - 1)
		      {
		        UR = 0;
		      }
		      else
		      {
		        UR = A[i - 1][j + 1];
		      }

		      if (j == 0)
		      {
		        CL = 0;
		      }
		      else
		      {
		        CL = A[i][j - 1];
		      }

		      CC = A[i][j];

		      if (j == N - 1)
		      {
		        CR = 0;
		      }
		      else
		      {
		        CR = A[i][j + 1];
		      }

		      if (i == N - 1 || j == 0)
		      {
		        LL = 0;
		      }
		      else
		      {
		        LL = A[i + 1][j - 1];
		      }

		      if (i == N - 1)
		      {
		        LC = 0;
		      }
		      else
		      {
		        LC = A[i + 1][j];
		      }

		      if (i == N - 1 || j == N - 1)
		      {
		        LR = 0;
		      }
		      else
		      {
		        LR = A[i + 1][j + 1];
		      }

		      if (i % 2 == 0)
		      {
		        if (j % 2 == 0)
		        { // periptosi ii
		          G[i][j] = (CC);
		          B[i][j] = (CL + CR) / 2;
		          R[i][j] = (UC + LC) / 2;
		        }
		        else
		        { // periptosi iv
		          B[i][j] = (CC);
		          R[i][j] = (UL + UR + LL + LR) / 4;
		          G[i][j] = (UC + LC + CL + CR) / 4;
		        }
		      }
		      else
		      {
		        if (j % 2 == 0)
		        { // periptosi iii
		          R[i][j] = (CC);
		          B[i][j] = (UL + UR + LL + LR) / 4;
		          G[i][j] = (UC + LC + CL + CR) / 4;
		        }
		        else
		        { // periptosi i
		          G[i][j] = (CC);
		          B[i][j] = (UC + LC) / 2;
		          R[i][j] = (CL + CR) / 2;
		        }
		      }
		    }
		  }

}

int compare(){
	u32 tmp ;
	int a ,b ;
	int i = 0;
	int counter =0;
	for (a =0 ; a <N;a++){
		for (b=0;b<N;b++){
			tmp = (R[a][b]<<16) +(G[a][b]<<8) + (B[a][b]);
			if (tmp !=Rxtest[i]) {counter++;}
			i++;
		}
	}

	return counter;
}
int main()
{
	Xil_DCacheDisable();
	int Status;
	XTime preExecCyclesFPGA = 0;
	XTime postExecCyclesFPGA = 0;
	XTime preExecCyclesSW = 0;
	XTime postExecCyclesSW = 0;

	u8 Value;

	u8 *TxBufferPtr;
	u32 *RxBufferPtr;

	TxBufferPtr = (u8 *)TX_BUFFER ;
	RxBufferPtr = (u32 *)RX_BUFFER;

	print("HELLO 1\r\n");

	// User application local variables

	init_platform();

	printf("Start\n\r");



	int i,j;

	int counter =1;



	for (i =0;i<N;i++){
		for (j=0; j<N; j++){
			A[i][j]=counter%256 ;
			counter++;

		}
	}

	for (i =0 ; i<N*N;i++){
		RxBufferPtr[i]=0;
	}

    // Step 1: Initialize TX-DMA Device (PS->PL)

	XAxiDma_Config *CfgPtr_TX;

	CfgPtr_TX = XAxiDma_LookupConfig(TX_DMA_ID);
		if (!CfgPtr_TX) {
			printf("No config found for %d\r\n", TX_DMA_ID);
			return XST_FAILURE;
		}

		Status = XAxiDma_CfgInitialize(&AxiDma_TX, CfgPtr_TX);
		if (Status != XST_SUCCESS) {
			printf("Initialization failed %d\r\n", Status);
			return XST_FAILURE;
		}

		if(XAxiDma_HasSg(&AxiDma_TX)){
			printf("Device configured as SG mode \r\n");
			return XST_FAILURE;
		}

		/* Disable interrupts, we use polling mode
		 */
		XAxiDma_IntrDisable(&AxiDma_TX, XAXIDMA_IRQ_ALL_MASK,
							XAXIDMA_DEVICE_TO_DMA);
		XAxiDma_IntrDisable(&AxiDma_TX, XAXIDMA_IRQ_ALL_MASK,
							XAXIDMA_DMA_TO_DEVICE);


    // Step 2: Initialize RX-DMA Device (PL->PS)
		XAxiDma_Config *CfgPtr_RX;
		CfgPtr_RX = XAxiDma_LookupConfig(RX_DMA_ID);
				if (!CfgPtr_RX) {
					xil_printf("No config found for %d\r\n", RX_DMA_ID);
					return XST_FAILURE;
				}

				Status = XAxiDma_CfgInitialize(&AxiDma_RX, CfgPtr_RX);
				if (Status != XST_SUCCESS) {
					xil_printf("Initialization failed %d\r\n", Status);
					return XST_FAILURE;
				}

				if(XAxiDma_HasSg(&AxiDma_RX)){
					xil_printf("Device configured as SG mode \r\n");
					return XST_FAILURE;
				}

				/* Disable interrupts, we use polling mode
				 */
				XAxiDma_IntrDisable(&AxiDma_RX, XAXIDMA_IRQ_ALL_MASK,
									XAXIDMA_DEVICE_TO_DMA);
				XAxiDma_IntrDisable(&AxiDma_RX, XAXIDMA_IRQ_ALL_MASK,
									XAXIDMA_DMA_TO_DEVICE);



	XTime_GetTime((XTime *) &preExecCyclesFPGA);


    // Step 3 : Perform FPGA processing

		Value = 0x01;
		xil_printf("TxBuffer\n");

		for (i=0; i<16; i++){

			TxBufferPtr[i]=Value;

			Value = (Value + 1) & 0xFF;
			printf("This is TxBufferPtr[%d]:%d\n\r",i,TxBufferPtr[i]);
		}





			xil_printf("Finish buffer\n");

    //      3a: Setup RX-DMA transaction
			printf("Start RX-DMA transaction\n");
			Status = XAxiDma_SimpleTransfer(&AxiDma_RX,(UINTPTR) RxBufferPtr,
							Max_packet_len, XAXIDMA_DEVICE_TO_DMA);

			if (Status != XST_SUCCESS) {
						return XST_FAILURE;
					}

			printf("Finish RX-DMA transaction\n");
    //      3b: Setup TX-DMA transaction
			printf("Setup TX-DMA transaction\n");
			Status = XAxiDma_SimpleTransfer(&AxiDma_TX,(UINTPTR) TxBufferPtr,
					Max_packet_len, XAXIDMA_DMA_TO_DEVICE);

			if (Status != XST_SUCCESS) {
					return XST_FAILURE;
				}
			printf("Finish TX-DMA transaction\n");


    //      3c: Wait for TX-DMA & RX-DMA to finish
			printf("Start waiting\n");
			i=0;

			while (XAxiDma_Busy(&AxiDma_TX,XAXIDMA_DMA_TO_DEVICE)) {
				/* Wait */
				if (i == 2000000){
					goto err;
				}
				i++;
			}

			printf("Finish_Tx\n");
			while (XAxiDma_Busy(&AxiDma_RX,XAXIDMA_DEVICE_TO_DMA)){
				if (i == 2000000){
					goto err1;
					}
				i++;

			}
			printf("Finish_Rx\n");
			i=0;

			printf("Finish waiting\n");
			goto finish;

	err: printf("This is err\n");
	err1 : printf("This is err1\n");

	finish:
		Rxtest = (u32 *)RX_BUFFER;
		for (i = 0;i<16;i++){
			printf("This is Rx[%d]:%llu\n",i,Rxtest[i]);
			//printf("This is Rx-B[%d]:%d\n",i,( Rxtest[i]& 0xff ) );
			//printf("This is Rx-G[%d]:%d\n",i,((Rxtest[i]&0xff00)>>8) );
			//printf("This is Rx-R[%d]:%d\n",i,((Rxtest[i]&0xff0000)>>16));
			//printf("This is Rx-empty[%d]:%d\n",i,((Rxtest[i]&0xff000000)>>24));
		}
		printf("RX buffer\n");



		XTime_GetTime((XTime *) &postExecCyclesFPGA);

		printf("This is post exefpga:%d\n\r",postExecCyclesFPGA);
		XTime_GetTime((XTime *) &preExecCyclesSW);
    // Step 5: Perform SW processing

		sw_fix();


		XTime_GetTime((XTime *) &postExecCyclesSW);

    for (i =0; i<N;i++){
    	for (j = 0;j<N;j++){
    		printf("%d,%d,%d\n\r",R[i][j],G[i][j],B[i][j]);
    	}
    }


     int errors ;
     errors = compare();
     XTime Exe_Fpga = postExecCyclesFPGA-preExecCyclesFPGA;

     XTime Exe_Sw = postExecCyclesSW-preExecCyclesSW;

     double RunTime = 1.0*(int)Exe_Fpga/(int)Exe_Sw;


    // Step 6: Compare FPGA and SW results
    //     6a: Report total percentage error
     	 	 printf("Percentage of error:%f\n\r",(1.0*errors)/(N*N));
    //     6b: Report FPGA execution time in cycles (use preExecCyclesFPGA and postExecCyclesFPGA)
     	 	printf("Start Fpga: %lld, Finish Fpga %lld, Execution time Fpga: %lld\n\r", preExecCyclesFPGA, postExecCyclesFPGA, Exe_Fpga);
    //     6c: Report SW execution time in cycles (use preExecCyclesSW and postExecCyclesSW)
     	 	printf("Start software: %lld, Finish software: %lld, Execution time Software: %lld\n\r", preExecCyclesSW, postExecCyclesSW, Exe_Sw);
    //     6d: Report speedup (SW_execution_time / FPGA_exection_time)
     	 	printf("Speedup : %f \n\r",RunTime);


    cleanup_platform();
    return 0;
}