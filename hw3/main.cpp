#include "mbed.h"
#include "mbed_rpc.h"
//#include "uLCD_4DGL.h"
#include "stm32l475e_iot01_accelero.h"

/**
 *  This example program has been updated to use the RPC implementation in the new mbed libraries.
 *  This example demonstrates using RPC over serial
**/
RpcDigitalOut myled1(LED1,"myled1");
RpcDigitalOut myled2(LED2,"myled2");
RpcDigitalOut myled3(LED3,"myled3");
BufferedSerial pc(USBTX, USBRX);
//uLCD_4DGL uLCD(D1, D0, D2);
InterruptIn but(USER_BUTTON);

int16_t pDataXYZ[3] = {0};
int idR[32] = {0};
int indexR = 0;



void Gesture_UI(Arguments *in, Reply *out);
RPCFunction rpcGesture_UI(&Gesture_UI, "Gesture_UI");
void gesture_ui();
void Angle_Detection(Arguments *in, Reply *out);
RPCFunction rpcAngle_Detection(&Angle_Detection, "Angle_Detection");
EventQueue gesture_queue(32 * EVENTS_EVENT_SIZE);
EventQueue angle_detection_queue(32 * EVENTS_EVENT_SIZE);
Thread gesture_thread;
Thread angle_detection_thread;
EventQueue queue(32 * EVENTS_EVENT_SIZE);       // for uLCD display
Thread thread;                                  // for uLCD display

double x, y;

int main() {
    thread.start(callback(&queue, &EventQueue::dispatch_forever));
    //gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));
    
    //The mbed RPC classes are now wrapped to create an RPC enabled version - see RpcClasses.h so don't add to base class
    // receive commands, and send back the responses
    char buf[256], outbuf[256];

    FILE *devin = fdopen(&pc, "r");
    FILE *devout = fdopen(&pc, "w");

    while(1) {
        memset(buf, 0, 256);
        for (int i = 0; ; i++) {
            char recv = fgetc(devin);
            if (recv == '\n') {
                printf("\r\n");
                break;
            }
            buf[i] = fputc(recv, devout);
        }
        //Call the static call method on the RPC class
        RPC::call(buf, outbuf);
        printf("%s\r\n", outbuf);
    }
}

void Gesture_UI(Arguments *in, Reply *out) {
    gesture_queue.call(gesture_ui);
    gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));
}

void gesture_ui() {
    for (int i=0; i<10; i++) {
        printf("testtesttest\r\n");
    }
}

void Angle_Detection(Arguments *in, Reply *out) {
    angle_detection_thread.start(callback(&angle_detection_queue, &EventQueue::dispatch_forever));
}




























//demo
// void LEDControl_GREEN (Arguments *in, Reply *out)   {
//     bool success = true;

//     // In this scenario, when using RPC delimit the two arguments with a space.
//     //x = in->getArg<double>();
//     y = in->getArg<double>();

//     // Have code here to call another RPC function to wake up specific led or close it.
//     char buffer[200], outbuf[256];
//     char strings[20];
//     //int led = x;
//     int on = y;
//     sprintf(strings, "/myled/write %d", on);
//     strcpy(buffer, strings);
//     RPC::call(buffer, outbuf);
//     if (success) {
//         out->putData(buffer);
//     } else {
//         out->putData("Failed to execute LED control.");
//     }
// }


// void BLINK(Arguments *in, Reply *out)   {
//     bool success = true;

//     // In this scenario, when using RPC delimit the two arguments with a space.
//     //x = in->getArg<double>();
//     //y = in->getArg<double>();

//     // Have code here to call another RPC function to wake up specific led or close it.
//     char buffer[200], outbuf[256];
//     char strings[20];
//     //int led = x;
//     while(1) {
//         RPC::call("/myled3/write 1", outbuf);
//         ThisThread::sleep_for(500ms);
//         RPC::call("/myled3/write 0", outbuf);
//         ThisThrexad::sleep_for(500ms);
//     }
    
// }