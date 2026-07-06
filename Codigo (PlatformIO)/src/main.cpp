#include <misc.h>
#include <objects.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

//=================================
//CONFIGS

//Program selector (0 = Bot | 1 = Controller)
#define PROGRAM_SELECT CONTROLLER_PROGRAM

#define CHAR_UUID "c0de0002-feed-babe-cafe-00000000000b"

//Cambiar aqui por cada bot y control (A-B-C-D)
#define BOT_NAME "BOT_B"
#define SERVICE_UUID "C0DE000B-1234-5678-ABCD-000000000000"

#define MAX_WHEEL_SPEED 250
#define SENSITIVITY 0.7
#define JOYSTICK_DEADZONE 2048

//=================================
//Globals
MotorShield motors;
Joystick joystick;

typedef struct{ 
	int x_power; 
	int y_power; 
	bool btn_pressed; 
} JoystickData;

JoystickData myData;
//=================================
//BLE Connection
static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHAR_UUID);

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic;


//----------------------------------------
//CONTROLLER

//Bot finder
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks{
  void onResult(BLEAdvertisedDevice advertisedDevice){
    if(advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)){
      Serial.println("¡Bot encontrado!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};


class MyClientCallback: public BLEClientCallbacks{
  void onConnect(BLEClient* pclient) {}
  
  void onDisconnect(BLEClient* pclient) {
    Serial.println("Bot perdido :c");
    connected = false; 
  }
};


//Linker function
bool connectToServer() {
  Serial.println("Conectando al servidor...");
  BLEClient* pClient = BLEDevice::createClient();
  
  // Le asignamos el callback de desconexión al cliente
  pClient->setClientCallbacks(new MyClientCallback());
  
  if (!pClient->connect(myDevice)) return false;
  
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }
  
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }
  
  return true;
}

//----------------------------------------
//BOT

//Connection Callback
class MyServerCallbacks: public BLEServerCallbacks{
    void onConnect(BLEServer* pServer){
      Serial.println("Joystick Connected!");
    };

    void onDisconnect(BLEServer* pServer){
      Serial.println("Lost signal :c");
		#if PROGRAM_SELECT == BOT_PROGRAM
			motors.stopMotors();
		#endif
      BLEDevice::startAdvertising(); 
    }
};


//Bot controller response
class MyCallbacks: public BLECharacteristicCallbacks{
	#if PROGRAM_SELECT == BOT_PROGRAM
		void onWrite(BLECharacteristic *pCharacteristic){
			std::string rxValue = pCharacteristic->getValue();
			
			if(rxValue.length() == sizeof(JoystickData)){
				JoystickData* myData = (JoystickData*)rxValue.data();
				
				int x_power = myData->x_power;
				int y_power = myData->y_power;
				bool btn_pressed = myData->btn_pressed;

				int forwardSpeed = map(y_power, -100, 100, -MAX_WHEEL_SPEED, MAX_WHEEL_SPEED);
				int steeringSpeed = map(x_power, -100, 100, -MAX_WHEEL_SPEED, MAX_WHEEL_SPEED);

				int leftMotor = forwardSpeed + steeringSpeed;
				int rightMotor = forwardSpeed - steeringSpeed;

				motors.controlMotors(leftMotor, rightMotor);
				Serial.printf("Joystick -> X:%hd Y:%hd | Motores -> L:%d R:%d\n", x_power, y_power, leftMotor, rightMotor);
			}
		}
	#endif
};



void setup(){
  delay(3000); 
  Serial.begin(115200);

  #if PROGRAM_SELECT == BOT_PROGRAM
  	Serial.println("Iniciando FutbolCRITO: " + String(BOT_NAME));
	motors.init(PWM_A_PIN, A1_PIN, A2_PIN, PWM_B_PIN, B1_PIN, B2_PIN, MAX_WHEEL_SPEED);

	//Init BLE server
	BLEDevice::init(BOT_NAME);
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	BLEService *pService = pServer->createService(SERVICE_UUID);
	
	BLECharacteristic *pCharacteristic = pService->createCharacteristic(
											CHAR_UUID,
											BLECharacteristic::PROPERTY_WRITE
										);
										
	pCharacteristic->setCallbacks(new MyCallbacks());
	pService->start();
	
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	BLEDevice::startAdvertising();
	
	Serial.println("¡Bot listo para recibir comandos!");

  #elif PROGRAM_SELECT == CONTROLLER_PROGRAM
    Serial.println("Iniciando mando de FutbolCRIto: " + String(BOT_NAME));
	joystick.init(VRX_PIN, VRY_PIN, SW_PIN, SENSITIVITY, JOYSTICK_DEADZONE);
		
	BLEDevice::init(String("Mando: " + String(BOT_NAME)).c_str());
	
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setInterval(1349);
	pBLEScan->setWindow(449);
	pBLEScan->setActiveScan(true);
	pBLEScan->start(5, false);

  #endif
}

void loop(){
	#if PROGRAM_SELECT == BOT_PROGRAM
		delay(100);

	#elif PROGRAM_SELECT == CONTROLLER_PROGRAM
		if(doConnect){
			if(connectToServer()){
				Serial.println("¡Conectado al bot!");
				connected = true;
			} 
			else{
				Serial.println("Bot no encontrado :c");
			}
			doConnect = false;
		}

		if(connected){
			myData.x_power = joystick.get_axis_power(X_AXIS);
			myData.y_power = joystick.get_axis_power(Y_AXIS);
			myData.btn_pressed = joystick.is_pressed();
			
			//Write data
			pRemoteCharacteristic->writeValue((uint8_t*)&myData, sizeof(myData));
			delay(50); 
		} 
		else{
			//Reconnect if lost
			BLEDevice::getScan()->start(5, false);
			delay(1000);
		}

	#else
		delay(100);

	#endif
}