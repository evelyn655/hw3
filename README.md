# Homework 3 TF Lite, WiFi and MQTT
此次作業的操作流程如下：
#### [ Part I ]
1. 在 screen 輸入指令 `/Gesture_UI/run`，呼叫RPC function: Gesture_UI
2. Gesture_UI 會 call thread function: gesture_ui，在 gesture_ui 中會進行手勢辨識，並依據辨識出的不同手勢，進行角度選擇。
3. 當使用者確定（按下 button）所選擇的角度後，mbed 會 publish 該數值至 topic1 "Angle selection"，同時 python 端有訂閱 topic1，所以會接收到訊息。
4. python 端接收到訊息後，會在 screen 寫入 `/Leave_Mode/run`，該 RPC function 使 mbed 跳出 Gesture_UI 模式。
#### [ Part II ]
5. 在 screen 輸入指令 `/Angle_Detection/run`，呼叫RPC function: Angle_Detection
6. Angle_Detection 會 call thread function: angle_detection，在 angle_detection 中會計算目前 mbed 的傾角。
7. 當 mbed 傾斜的角度超過 [ Part I ] 中所選擇的角度，mbed 會 publish 訊息至 topic2 "Angle detection"，同時 python 端有訂閱 topic2，所以會接收到訊息。
8. python 端接收到 10 次訊息後，會在 screen 寫入 `/Leave_Mode/run`，該 RPC function 使 mbed 跳出 Angle_Detection 模式。

## To setup and run the program 
### main.cpp 
#### [ main function ]
* uLCD 初始顯示設定：
    顯示三個角度選項（15°、45°、60°）
```
    uLCD.background_color(WHITE);         // initial display on uLCD
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
```
* wifi、MQTT 連接
    * wifi、MQTT 連接
    如果沒有成功連上 wifi 或 MQTT，會 print 出 error 訊息。
    * client1 及 topic1 設定
        client1 負責 publish 選擇到的角度到 topic1: ```"Angle selection"```
    * InturruptIn:
        按下 button，會進入 publish_message1 函式中進行 publish
     ```btn.rise(mqtt_queue.event(&publish_message1, &client1)); ```
    
* RPC loop
    不斷讀取從 screen 輸入的 RPC 指令
```
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
```
* others:
    * accelerator initialization:  ```BSP_ACCELERO_Init();```

#### [ Part I - Gesture_UI mode ]
* RPC function: ```Gesture_UI(Arguments *in, Reply *out)```
    * LED 指示燈閃爍
    * 打開 thread 並呼叫 thread function
    ```gesture_thread.start(callback(&gesture_queue, &EventQueue::dispatch_forever));```
    ```gesture_queue.call(gesture_ui);```     
* thread function: ```gesture_ui()```
    * 手勢偵測
        * （同 lab 內容），並依據偵測到的手勢進行角度選擇。
        * ``` print_angle ``` 負責 uLCD 的顯示。
        * 在 while 迴圈中不斷偵測角度，直到 num 的值被改變為止。（在進入 ```gesture_ui()``` function 時，將 num 設成 1）
     
        ```
         while (num==1) {
         
             (******** Omit the code here for simplyfing *************)
             
            if (gesture_index < label_num) {
                error_reporter->Report(config.output_message[gesture_index]);
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
            }
        }
        ```





#### [ Part II - Angle_Detection mode ]
* RPC function: ```Angle_Dectection(Arguments *in, Reply *out)```
* thread function: ```angle_detection()```


###  mqtt_client.py 

## The results

## 問題紀錄、解決
* while return -1
* python global
