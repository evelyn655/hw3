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
### 以下為主要功能的實現方法。
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
        * 在 while 迴圈中不斷偵測角度，直到 num 的值被改變為止。（在每次進入 ```gesture_ui()``` function 時，會將 num 設成 1）
     
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
    * LED 指示燈閃爍
    * 打開 thread 並呼叫 thread function  ```angle_detection_thread.start(callback(&angle_detection_queue,&EventQueue::dispatch_forever));```
```angle_detection_queue.call(angle_detection);```
* thread function: ```angle_detection()```
    * client2 及 topic2 設定
        client2 負責在 Mbed 的傾角超過在 [Part - I] 所選擇的角度時， publish 當時的傾角資訊至 topic2: ```"Angle detection"```
        client2 也需要 MQTT 的連接設定，方法和 main function 中相同
    * 傾角計算、publish    
        * ```pDataXYZ_init``` 存入參考向量 A（Mbde 平放於桌面時）
        * ```pDataXYZ``` 存入 Mbed 傾斜時所測到的向量 B，由三維空間的內積計算，可以得到 Mbed 目前的傾角: ```angle_det```
        * print_angle_detect 負責 uLCD 的顯示。
        * 如果計算出的 ```angle_det``` 大於 [Part - I] 所選擇的角度 ```angle_sel```，會進入 publish_message2 函式中進行 publish（在另一個 thread 中執行，所以不會互相干擾）
        * 在 while 迴圈中不斷計算傾角，直到 num 的值被改變為止。（在每次進入 angle_detection() function 時，會將 num 設成 2）
        ```
        BSP_ACCELERO_AccGetXYZ(pDataXYZ_init);
        printf("reference acceleration vector: %d, %d, %d\r\n", pDataXYZ_init[0], pDataXYZ_init[1], pDataXYZ_init[2]);


        (******** Omit the code here for simplyfing *************)


        num = 2;             // tilt Angle_Detection mode
        while (num==2) {
            BSP_ACCELERO_AccGetXYZ(pDataXYZ);
            printf("Angle Detection: %d %d %d\r\n",pDataXYZ[0], pDataXYZ[1], pDataXYZ[2]);
            mag_A = sqrt(pDataXYZ_init[0]*pDataXYZ_init[0] + pDataXYZ_init[1]*pDataXYZ_init[1] + pDataXYZ_init[2]*pDataXYZ_init[2]);
            mag_B = sqrt(pDataXYZ[0]*pDataXYZ[0] + pDataXYZ[1]*pDataXYZ[1] + pDataXYZ[2]*pDataXYZ[2]);
            cos = ((pDataXYZ_init[0]*pDataXYZ[0] + pDataXYZ_init[1]*pDataXYZ[1] + pDataXYZ_init[2]*pDataXYZ[2])/(mag_A)/(mag_B));
            rad_det = acos(cos);
            angle_det = 180.0 * rad_det/PI;
            printf("angle_det = %.2f\r\n", angle_det);
            queue.call(print_angle_detect);

            if (angle_det > angle_sel) {
                mqtt_queue.call(&publish_message2, &client2);
            }
            ThisThread::sleep_for(1000ms);
        }
        ```

#### [ Leave_Mode ]
當 mqtt_client.py 收到訂閱 topics 的訊息後，會從 python 端寫入此 RPC 指令 ```/Leave_Mode/run\r\n```，並執行以下事項。
* RPC function:  ```Leave_Mode(Arguments *in, Reply *out)```
將 Global 的函數 num 改成 0 ，使得 [Part - I] 及 [Part - II] 中的 while 迴圈都結束執行。
        ```
        printf("\r\nSuccessfully leave the mode\r\n");
        num = 0;
        ```



###  mqtt_client.py
* Topics 設定及訂閱
```
topic1 = "Angle selection"
topic2 = "Angle detection"
```
```
mqttc.subscribe(topic1, 0)
mqttc.subscribe(topic2, 0)
```
* 收到 message 後的處理
    * 在需要時發送 RPC 指令: ```/Leave_Mode/run\r\n```，使 Mbed 離開當下的 mode（Gesture_UI mode 或 Angle_Detection mode）
        * 當第一次收到 publish 的訊息（Gesture_UI mode 的角度選擇），就在 screen 寫入```/Leave_Mode/run\r\n``` 跳出該 mode
        * 之後收到的 publish 來自 Angle_Detection mode，若 Mbed 的傾角大於選擇的角度時， python 會接收到 publish 的訊息並印出，當接收到 10 次 messages 後，就在 screen 寫入```/Leave_Mode/run\r\n``` 跳出該 mode
   
```
def on_message(mosq, obj, msg):
    global flag, count
    print("flag: ", flag)
    num = str(msg.payload)
    if flag == 0:
        print("Message: " + str(msg.payload) + "\n")
        s.write(bytes("/Leave_Mode/run\r\n", 'UTF-8'))
        flag = 1
    else:
        if count != 9:
            count+=1
            print("count: ", count)
            print("Message: " + str(msg.payload) + "\n")
        else:
            print("Message: " + str(msg.payload) + "\n")
            s.write(bytes("/Leave_Mode/run\r\n", 'UTF-8'))
            count = 0
            flag = 0
```

## The results
* Gesture_UI mode
    * Screen（上圖）會顯示資訊、偵測到的手勢。
    * 按下 publish 後，python 端（下圖）會收到選擇的角度資訊，並在 screen 端寫入`/Leave_Mode/run`，停止 Gesture_UI mode。　
    ![](https://i.imgur.com/x4UYMpw.png)![](https://i.imgur.com/IloQ4Dn.png)
    
* Angle_Deteciton mode
    * Screen 會顯示三軸加速器相關的量測資訊，並印出傾角計算結果。如果傾角大於 [Part - I] 所選擇的角度，則會將該角度資訊 publish 至 python 端（下圖）。
    ![](https://i.imgur.com/faMUHhv.png)![](https://i.imgur.com/6oJpcSJ.png)
    * 左圖為 screen 畫面，右圖為 python 畫面
    ![](https://i.imgur.com/GRNQtoM.png)






## 問題紀錄、解決
* thread function 無法停止：                                 
    一開始遇到比較大的問題是 gesture_ui function 一直執行，無法停下來。                                                    
    後來發現的可能是因為程式碼中的 return -1，造成有些部分無法如預期往下面的程式碼執行。                                     
    將部分 return -1 註解掉後，即可如預期的完成功能。

* 程式執行時產生 HardFault，解決方法是將以下幾行的宣告和設定:
    ```
    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);
    ```
    由 Global 放到會呼叫 publish_message 的 function 裡，即可解決此 error。                    
    但有點不太確定原因為何。
    
* python globa variable：                                        
    如果在 function 中有改變 global variables 的必要，則需要在該 function 中特別再宣告一次: `global flag, count` ，否則執行時會遇到 error 訊息: `UnboundLocateError: local variable 'flag' referenced before assignment`
