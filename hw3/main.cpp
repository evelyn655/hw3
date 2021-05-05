#include "mbed.h"
#include "mbed_rpc.h"
#include "uLCD_4DGL.h"
#include "stm32l475e_iot01_accelero.h"


// for Machine Learning on mbed
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"


// for MQTT
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

/**
 *  This example program has been updated to use the RPC implementation in the new mbed libraries.
 *  This example demonstrates using RPC over serial
**/
//RpcDigitalOut myled1(LED1,"myled1");
//RpcDigitalOut myled2(LED2,"myled2");
//RpcDigitalOut myled3(LED3,"myled3");
DigitalOut myled1(LED1);
BufferedSerial pc(USBTX, USBRX);
uLCD_4DGL uLCD(D1, D0, D2);
InterruptIn btn(USER_BUTTON);

void print_angle();

void Gesture_UI(Arguments *in, Reply *out);
RPCFunction rpcGesture_UI(&Gesture_UI, "Gesture_UI");
void ANGLE_SEL();
int gesture_ui();

void Angle_Detection(Arguments *in, Reply *out);
RPCFunction rpcAngle_Detection(&Angle_Detection, "Angle_Detection");

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client);

EventQueue gesture_queue(32 * EVENTS_EVENT_SIZE);
//EventQueue gesture_queue1(32 * EVENTS_EVENT_SIZE);
EventQueue angle_detection_queue(32 * EVENTS_EVENT_SIZE);
//EventQueue angle_detection_queue1(32 * EVENTS_EVENT_SIZE);
EventQueue queue(32 * EVENTS_EVENT_SIZE);                               // for uLCD display
Thread gesture_thread;
//Thread gesture_thread1;
Thread angle_detection_thread;
//Thread angle_detection_thread1;
Thread thread;                                                          // for uLCD display
// for MQTT
Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;

// for Accelerometer
// int16_t pDataXYZ[3] = {0};
// int idR[32] = {0};
// int indexR = 0;

// for MQTT
// GLOBAL VARIABLES
WiFiInterface *wifi;
volatile int message_num = 0;
volatile int arrivedcount = 0;
volatile bool closed = false;

// NetworkInterface* net = wifi;
// MQTTNetwork mqttNetwork(net);
// MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

const char* topic = "Mbed";

// for Gesture_UI mode
int angle = 15;
int angle_sel = 15;

/*BELOW: MQTT function*/
/*************************************************************************************/
/*************************************************************************************/
void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
    char msg[300];
    sprintf(msg, "Message arrived: QoS%d, retained %d, dup %d, packetID %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf(msg);
    ThisThread::sleep_for(1000ms);
    char payload[300];
    sprintf(payload, "Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    printf(payload);
    ++arrivedcount;
}

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {

    angle_sel = angle;
    printf("angle_sel = %d\r\n", angle_sel);

    message_num++;
    MQTT::Message message;
    char buff[100];
    
    sprintf(buff, "Angle Selected: %d", angle_sel);
    
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buff;
    message.payloadlen = strlen(buff) + 1;
    int rc = client->publish(topic, message);

    printf("rc:  %d\r\n", rc);
    printf("Puslish message: %s\r\n", buff);
}

void close_mqtt() {
    closed = true;
}
/*************************************************************************************/
/*************************************************************************************/
/*Above: MQTT function*/





/*BELOW: Machine Learning on mbed*/
/*************************************************************************************/
/*************************************************************************************/
// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// Return the result of the last prediction
int PredictGesture(float* output) {
  // How many times the most recent gesture has been matched in a row
  static int continuous_count = 0;
  // The result of the last prediction
  static int last_predict = -1;

  // Find whichever output has a probability > 0.8 (they sum to 1)
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }

  // No gesture was detected above the threshold
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }

  if (last_predict == this_predict) {
    continuous_count += 1;
  } else {
    continuous_count = 0;
  }
  last_predict = this_predict;

  // If we haven't yet had enough consecutive matches for this gesture,
  // report a negative result
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  // Otherwise, we've seen a positive result, so clear all our variables
  // and report it
  continuous_count = 0;
  last_predict = -1;

  return this_predict;
}
/*************************************************************************************/
/*************************************************************************************/
/*Above: Machine Learning on mbed*/


// for display on uLCD
void print_angle() {
    //uLCD.cls();
    uLCD.color(BLACK);
    uLCD.locate(1, 2);
    uLCD.printf("\n15\n");    
    uLCD.locate(1, 4);
    uLCD.printf("\n45\n");
    uLCD.locate(1, 6);
    uLCD.printf("\n60\n");
    

    if (angle==15) {
        uLCD.color(RED);
        uLCD.locate(1, 2);
        uLCD.printf("\n15\n");
    } else if (angle==45) {
        uLCD.color(RED);
        uLCD.locate(1, 4);
        uLCD.printf("\n45\n");
    } else if (angle==60) {
        uLCD.color(RED);
        uLCD.locate(1, 6);
        uLCD.printf("\n60\n");
    } 
}


void Gesture_UI(Arguments *in, Reply *out) {
    for (int i=0; i<5; i++) {
        myled1 = 1;                            // use LED1 to indicate the start of UI mode
        ThisThread::sleep_for(100ms);
        myled1 = 0;
        ThisThread::sleep_for(100ms);
    }
    gesture_queue.call(gesture_ui);
    gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));
}

// void ANGLE_SEL() {
//     angle_sel = angle;
//     printf("angle_sel = %d\r\n", angle_sel);

// } 



// main function in lab08
int gesture_ui() {
    // for (int i=0; i<10; i++) {
    //     printf("testtesttest\r\n");
    // }

    // Whether we should clear the buffer next time we fetch data
    bool should_clear_buffer = false;
    bool got_data = false;

    // The gesture index of the prediction
    int gesture_index;
    // int angle = 15;
    // int angle_sel = 15;



/*BELOW: Machine Learning on mbed*/
/*************************************************************************************/
/*************************************************************************************/
    // Set up logging.
    static tflite::MicroErrorReporter micro_error_reporter;
    tflite::ErrorReporter* error_reporter = &micro_error_reporter;

    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        error_reporter->Report(
            "Model provided is schema version %d not equal "
            "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return -1;
    }

    // Pull in only the operation implementations we need.
    // This relies on a complete list of all the ops needed by this graph.
    // An easier approach is to just use the AllOpsResolver, but this will
    // incur some penalty in code space for op implementations that are not
    // needed by this graph.
    static tflite::MicroOpResolver<6> micro_op_resolver;
    micro_op_resolver.AddBuiltin(
        tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
        tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                                 tflite::ops::micro::Register_MAX_POOL_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                                 tflite::ops::micro::Register_CONV_2D());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                                 tflite::ops::micro::Register_FULLY_CONNECTED());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                                 tflite::ops::micro::Register_SOFTMAX());
    micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                                 tflite::ops::micro::Register_RESHAPE(), 1);

    // Build an interpreter to run the model with
    static tflite::MicroInterpreter static_interpreter(
        model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
    tflite::MicroInterpreter* interpreter = &static_interpreter;

    // Allocate memory from the tensor_arena for the model's tensors
    interpreter->AllocateTensors();

    // Obtain pointer to the model's input tensor
    TfLiteTensor* model_input = interpreter->input(0);
    if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
        (model_input->dims->data[1] != config.seq_length) ||
        (model_input->dims->data[2] != kChannelNumber) ||
        (model_input->type != kTfLiteFloat32)) {
        error_reporter->Report("Bad input tensor parameters in model");
        return -1;
    }

    int input_length = model_input->bytes / sizeof(float);

    TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
    if (setup_status != kTfLiteOk) {
        error_reporter->Report("Set up failed\n");
        return -1;
    }

    error_reporter->Report("Set up successful...\n");

    while (true) {

        // Attempt to read new data from the accelerometer
        got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                     input_length, should_clear_buffer);

        // If there was no new data,
        // don't try to clear the buffer again and wait until next time
        if (!got_data) {
            should_clear_buffer = false;
            continue;
        }

        // Run inference, and report any error
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            error_reporter->Report("Invoke failed on index: %d\n", begin_index);
            continue;
        }

        // Analyze the results to obtain a prediction
        gesture_index = PredictGesture(interpreter->output(0)->data.f);

        // Clear the buffer next time we read data
        should_clear_buffer = gesture_index < label_num;

        // Produce an output
        if (gesture_index < label_num) {
            error_reporter->Report(config.output_message[gesture_index]);
        }

        if (gesture_index == 0) {
            angle = 15;
            queue.call(print_angle);
        }
        else if (gesture_index == 1) {
            angle = 45;
            queue.call(print_angle);
        }
        else if (gesture_index == 2) {
            angle = 60;
            queue.call(print_angle);
        }
        //printf("%d\r\n", angle_sel);
        //btn.rise(mqtt_queue.event(&publish_message, &client));
        //btn.rise(&);
/*************************************************************************************/
/*************************************************************************************/
/*Above: Machine Learning on mbed*/



    } 
}


void Angle_Detection(Arguments *in, Reply *out) {
    angle_detection_thread.start(callback(&angle_detection_queue, &EventQueue::dispatch_forever));
}










//double x, y;

int main() {
    // display on uLCD
    uLCD.background_color(WHITE);               
    uLCD.cls();
    uLCD.textbackground_color(WHITE);
    uLCD.color(RED);
    uLCD.locate(1, 2);
    uLCD.printf("\n15\n");
    uLCD.color(BLACK);
    uLCD.locate(1, 4);
    uLCD.printf("\n45\n");
    uLCD.locate(1, 6);
    uLCD.printf("\n60\n");


    thread.start(callback(&queue, &EventQueue::dispatch_forever));
    mqtt_thread.start(callback(&mqtt_queue, &EventQueue::dispatch_forever));
    //gesture_thread1.start(callback(&gesture_queue1, &EventQueue::dispatch_forever));


    /*BELOW: MQTT*/
    /*************************************************************************************/
    /*************************************************************************************/
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
            printf("ERROR: No WiFiInterface found.\r\n");
            return -1;
    }

    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
            printf("\nConnection error: %d\r\n", ret);
            return -1;
    }
    
    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);


    //TODO: revise host to your IP
    const char* host = "172.20.10.2";
    printf("Connecting to TCP network...\r\n");

    SocketAddress sockAddr;
    sockAddr.set_ip_address(host);
    sockAddr.set_port(1883);

    printf("address is %s/%d\r\n", (sockAddr.get_ip_address() ? sockAddr.get_ip_address() : "None"),  (sockAddr.get_port() ? sockAddr.get_port() : 0) ); //check setting

    int rc = mqttNetwork.connect(sockAddr);//(host, 1883);
    if (rc != 0) {
            printf("Connection error.");
            return -1;
    }
    printf("Successfully connected!\r\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0){
            printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
            printf("Fail to subscribe\r\n");
    }

    
    /*************************************************************************************/
    /*************************************************************************************/
    /*Above: MQTT*/

    btn.rise(mqtt_queue.event(&publish_message, &client));


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





    

    int num = 0;
    while (num != 5) {
            client.yield(100);
            ++num;
    }

    while (1) {
            if (closed) break;
            client.yield(500);
            ThisThread::sleep_for(500ms);
    }

    printf("Ready to close MQTT Network......\n");

    if ((rc = client.unsubscribe(topic)) != 0) {
            printf("Failed: rc from unsubscribe was %d\n", rc);
    }
    if ((rc = client.disconnect()) != 0) {
    printf("Failed: rc from disconnect was %d\n", rc);
    }

    mqttNetwork.disconnect();
    printf("Successfully closed!\n");
    
    
}

























