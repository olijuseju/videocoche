/*Proyecto de control de coche 
 * 
 * Autor: Carlos Atienza Arnas 
 * 
 * Ordenes de bluetooth: 
 *    -St1 :Parada
 *    -St0 :Encendido
 *    -Ve(Velocidad):(Desviación de -1 a 1) :Cambio de referencia
 *    -Ac(Aceleración) : Cambio de aleleración
 *    
 * 07/03/2022: Conexion bluetooth acabad
 * 09/03/2022: Arreglado los pins no funcionales
 * TO DO:
 */

#include <Arduino.h>
#include <ESPmDNS.h>
#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

//----------------------Definicion encoders---------------------------
#define M1_encoder_A  27
#define M2_encoder_A  25
#define M3_encoder_A  32
#define M4_encoder_A  34
#define M1_encoder_B  26
#define M2_encoder_B  23
#define M3_encoder_B  33
#define M4_encoder_B  35

//-----------------------Definiciones comunes-------------------------
#define resolution  10
#define res_enc 400 //301
#define error_t 50 //en segundos
#define frec     40000
#define pin_S   25

//------------------------Definicion de motores-----------------------
//Motor 1
  //directo
#define M1_CM_ch_dir     0
#define M1_CM_pin_dir    2
  //inverso
#define M1_CM_ch_inv     1
#define M1_CM_pin_inv    4
//Motor 2
  //directo
#define M2_CM_ch_dir     2
#define M2_CM_pin_dir    12
  //inverso
#define M2_CM_ch_inv     3
#define M2_CM_pin_inv    13
//Motor 3
  //directo
#define M3_CM_ch_dir     4
#define M3_CM_pin_dir    18
  //inverso
#define M3_CM_ch_inv     5
#define M3_CM_pin_inv    19
//Motor 4
  //directo
#define M4_CM_ch_dir     6
#define M4_CM_pin_dir    22
  //inverso
#define M4_CM_ch_inv     7
#define M4_CM_pin_inv    21


//------------------------Variables comunes---------------------
//Variables de tareas
TaskHandle_t Task2;

//Variables de conexion
long Tc=100; //periodo de conexión
String Btin;   //La orden de entrada por bluetooth
String Ord;    //la orden anterior procesada
String Bton;    //la orden anterior reducida
char *s;  
bool conect=false;  //Control de conexion
bool ReadOk=false;  //Control de conexion pasada No se usa

//Variables generales
float er=0.0;       //Error entre la entrada y la referencia de la velocidad
float Cm_ref=0.01;  // Cambio de referencia por segundo
float kp=0.3;//1.9; // Proporcional
float ki=0.4;//1.5;//3;//7; //Integral
float Uk;     // Entrada del sistema
float Uik;    //  Entrada anterior del sistema
float Ik=0.0; //Integral del sistema
float acc = 0.0 ; //accion de control
uint32_t duty;  //la señal de envia
float Desv=0;// Direccion
//Referencias de velocidad del sistema
float Ref_iz;
float Ref_der;
float Ref_des=0;


//variables temporales
long t1 = 0 ;
long t2 = 0 ;
long t3=0;
long t4=0;
int T = 150;//57  ;                      // Periodo de muestreo milisegundos
float Tm = T/1000.0 ;              // Periodo de muestreo segundos

//Variables de conversión
float radio=2.5;
float C2RS = (2*3.14/res_enc )/Tm ;  // Conversión cuentas a rad/seg  
float RS2MS=radio/(2*3.14);
float CS2P=839/(RS2MS*(100*3.14/30));       // Convertir la M1_velocidad calculada a pulsos

// Variables de seguridad
uint32_t DutyMax = 1024;
uint32_t DutyMin = 170;
float v_max=RS2MS*(100*3.14/30);
float v_min=-v_max;

//Variables de control
bool Stop=true;

//-----------------------Variables propias del motor------------------------------
  //MOTOR 1
  //Variables de control
float M1_Ik_1=0.0;
float M1_ref=0.0;
float M1_vel = 0.0 ;
long M1_quad = 2241 ;
long M1_enc_ant = 0 ;
long M1_enc_act = 0 ;
int M1_enc_dif = 0 ;

//MOTOR 2
  //Variables de control
float M2_Ik_1=0.0;
float M2_ref=0.0;
float M2_vel = 0.0 ;
long M2_quad = 2241 ;
long M2_enc_ant = 0 ;
long M2_enc_act = 0 ;
int M2_enc_dif = 0 ;

//MOTOR 3
  //Variables de control
float M3_Ik_1=0.0;
float M3_ref=0.0;
float M3_vel = 0.0 ;
long M3_quad = 2241 ;
long M3_enc_ant = 0 ;
long M3_enc_act = 0 ;
int M3_enc_dif = 0 ;

//MOTOR 4
  //Variables de control
float M4_Ik_1=0.0;
float M4_ref=0.0;
float M4_vel = 0.0 ;
long M4_quad = 2241 ;
long M4_enc_ant = 0 ;
long M4_enc_act = 0 ;
int M4_enc_dif = 0 ;

//Variables test
float Ar=2;//C2RS*3;
int Tr=30000;
float eracum=0;
int Pos=0;
int test1=0;
int test2=0;
int test3=0;
int test4=0;

//-----------------------------------------Interrupciones del encoder-----------------------------------------
void M1_Fnc_encoder_A (){if(digitalRead(M1_encoder_A)==digitalRead(M1_encoder_B)){M1_quad--;}else{M1_quad++;}}
void M2_Fnc_encoder_A (){if(digitalRead(M2_encoder_A)==digitalRead(M2_encoder_B)){M2_quad--;}else{M2_quad++;}}
void M3_Fnc_encoder_A (){if(digitalRead(M3_encoder_A)==digitalRead(M3_encoder_B)){M3_quad--;}else{M3_quad++;}}
void M4_Fnc_encoder_A (){if(digitalRead(M4_encoder_A)==digitalRead(M4_encoder_B)){M4_quad--;}else{M4_quad++;}}
void M1_Fnc_encoder_B (){if(digitalRead(M1_encoder_A)==digitalRead(M1_encoder_B)){M1_quad++;}else{M1_quad--;}}
void M2_Fnc_encoder_B (){if(digitalRead(M2_encoder_A)==digitalRead(M2_encoder_B)){M2_quad++;}else{M2_quad--;}}
void M3_Fnc_encoder_B (){if(digitalRead(M3_encoder_A)==digitalRead(M3_encoder_B)){M3_quad++;}else{M3_quad--;}}
void M4_Fnc_encoder_B (){if(digitalRead(M4_encoder_A)==digitalRead(M4_encoder_B)){M4_quad++;}else{M4_quad--;}}


//----------------------------------------Actualizar la Salida del PWM ---------------------------------------------
float Actualizamot(float ref, float vel, float Ik_1,int chdir, int chfor){
  //Calculo de la accion utilizada
  er=ref-vel;
  Ik=T*er/1000.0+Ik_1;
  Uik=Ik*ki;
  Uk=kp*er ;
  acc=Uk+Uik;
  Serial.print(ref) ;Serial.print(" ") ;Serial.print(vel) ;Serial.print(" ") ;Serial.print(er) ; Serial.println(" ") ;

  //Seguridad de la velocidad
  if (acc>v_max)
    acc=v_max;
  if (acc<v_min)
    acc=v_min;

  //Calculo y seguridad del duty
  duty=(uint32_t)(abs(acc)*CS2P)+DutyMin;
  if (duty>DutyMax){
  duty=DutyMax;
  }else{
    if (duty<DutyMin){
      duty=DutyMin;}
 }
 

  //Escritura del PWM
  if (acc<0){
      ledcWrite(chdir, duty);
      ledcWrite(chfor, 0);
  }else{
      ledcWrite(chdir, 0);
      ledcWrite(chfor, duty);
      
  }
  return Ik;
  }


//--------------------------------------Cambia la referencia de los motores---------------------------------  
  void ActualizaDir(){
  int found = 0;
  int positionS = 1;
  int maxIndex = Bton.length()-1;
  for(int i=0; i<=maxIndex && found==0; i++){
    if(Bton.charAt(i)==':' ){
        found++;
        positionS=i;
    }
  }

   Ref_des=Bton.substring(0,positionS).toFloat();  
   Desv=Bton.substring(positionS+1,Btin.length()).toFloat();

   Serial.print("Btin ");
   Serial.println(Btin);
   Serial.print("Ref_des ");
   Serial.println(Ref_des);
   Serial.print("Desv ");
   Serial.println(Desv);

   
  }
  
//-------------------------------------------Inicializacion del dispositivo----------------------
void setup() {

  xTaskCreatePinnedToCore(
    loop2,
    "Task_2",
    1000,
    NULL,
    1,
    &Task2,
    0);
 
  Serial.begin(115200);
  SerialBT.begin("Videocoche"); //Bluetooth device name

  //Definicion encoder
  pinMode(M1_encoder_A, INPUT_PULLUP);
  pinMode(M2_encoder_A, INPUT_PULLUP);
  pinMode(M3_encoder_A, INPUT_PULLUP);
  pinMode(M4_encoder_A, INPUT_PULLUP);
  pinMode(M1_encoder_B, INPUT_PULLUP);
  pinMode(M2_encoder_B, INPUT_PULLUP);
  pinMode(M3_encoder_B, INPUT_PULLUP);
  pinMode(M4_encoder_B, INPUT_PULLUP);
  
  //Definición de las interrupciones
  attachInterrupt(M1_encoder_A,M1_Fnc_encoder_A,CHANGE) ; 
  attachInterrupt(M2_encoder_A,M2_Fnc_encoder_A,CHANGE) ; 
  attachInterrupt(M3_encoder_A,M3_Fnc_encoder_A,CHANGE) ; 
  attachInterrupt(M4_encoder_A,M4_Fnc_encoder_A,CHANGE) ; 
  attachInterrupt(M1_encoder_B,M1_Fnc_encoder_B,CHANGE) ; 
  attachInterrupt(M2_encoder_B,M2_Fnc_encoder_B,CHANGE) ; 
  attachInterrupt(M3_encoder_B,M3_Fnc_encoder_B,CHANGE) ; 
  attachInterrupt(M4_encoder_B,M4_Fnc_encoder_B,CHANGE) ; 
  //Definicion de los canales de los motores
    //Motor 1  
      //directo
  ledcSetup(M1_CM_ch_dir, frec, resolution);
  ledcAttachPin(M1_CM_pin_dir, M1_CM_ch_dir);
    //inverso
  ledcSetup(M1_CM_ch_inv, frec, resolution);
  ledcAttachPin(M1_CM_pin_inv, M1_CM_ch_inv);
   //Motor 2  
      //directo
  ledcSetup(M2_CM_ch_dir, frec, resolution);
  ledcAttachPin(M2_CM_pin_dir, M2_CM_ch_dir);
    //inverso
  ledcSetup(M2_CM_ch_inv, frec, resolution);
  ledcAttachPin(M2_CM_pin_inv, M2_CM_ch_inv);
   //Motor 3  
      //directo
  ledcSetup(M3_CM_ch_dir, frec, resolution);
  ledcAttachPin(M3_CM_pin_dir, M3_CM_ch_dir);
    //inverso
  ledcSetup(M3_CM_ch_inv, frec, resolution);
  ledcAttachPin(M3_CM_pin_inv, M3_CM_ch_inv);
   //Motor 4  
      //directo
  ledcSetup(M4_CM_ch_dir, frec, resolution);
  ledcAttachPin(M4_CM_pin_dir, M4_CM_ch_dir);
    //inverso
  ledcSetup(M4_CM_ch_inv, frec, resolution);
  ledcAttachPin(M4_CM_pin_inv, M4_CM_ch_inv);

  //Inicializacion final
  //Stop=false;
  er=0.0;
  
  ledcWrite(M1_CM_ch_dir, 0);
  ledcWrite(M1_CM_ch_inv, 0);
  ledcWrite(M2_CM_ch_dir, 0);
  ledcWrite(M2_CM_ch_inv, 0);
  ledcWrite(M3_CM_ch_dir, 0);
  ledcWrite(M3_CM_ch_inv, 0);
  ledcWrite(M4_CM_ch_dir, 0);
  ledcWrite(M4_CM_ch_inv, 0);

  SerialBT.println("Ready");
}
//--------------------------------------------------------Bucle de comunicaciones----------------------------------------
void loop2(void *parameter){
  for(;;){
    if (SerialBT.available()) {    
     Btin=SerialBT.readString();
     if(Btin == "Status"){
       SerialBT.print("St:"+String(abs(Stop-1))+":"+String(Cm_ref));
       conect=true;
       t4=millis();
    }else if (Btin =="live"){
         conect=true;
         t4=millis();
    }else{
       //Serial.println(Cm_ref);
       Serial.print(Btin);Serial.println(" // Orden completa:");
       //Btin.remove(Btin.length()-2,2);
       
       Ord=Btin.substring(0,2);
       Bton=Btin.substring(2,Btin.length());
       Serial.print(Ord); Serial.println(" // prefijo:" );
       //Btin.remove(0,2);
       Serial.print(Bton); Serial.println(" // Ordenreducida:" );
       //Serial.println(Btin); 
       if (Ord == "St"){
    
         if(Bton == "0")
           Stop=0;
         else
           Stop=1;
           }
    
        if (Ord == "Ve"){
          ActualizaDir();
        }
    
        if (Ord == "Ac"){
          Cm_ref=Bton.toFloat();
          Serial.print(Cm_ref); Serial.println(" // Aceleracion:" );


        }
      }
      ReadOk=true;
    //Serial.print(ReadOk); Serial.print(":");
   t3=millis() ;
 }
 
    delay(1); 
  }
  vTaskDelay(10);
}
//--------------------------------------------------------Bucle de control-----------------------------------------------
void loop() {

  //Lectura de las velocidades
    //Motor 1     
  M1_enc_act=M1_quad ;
  M1_enc_dif=M1_enc_act-M1_enc_ant ;
  M1_vel=(M1_enc_dif)*C2RS*RS2MS ;
  M1_enc_ant=M1_enc_act ;       
   //Motor 2     
  M2_enc_act=M2_quad ;
  M2_enc_dif=M2_enc_ant-M2_enc_act ;
  M2_vel=(M2_enc_dif)*C2RS*RS2MS ;
  M2_enc_ant=M2_enc_act ;      
   //Motor 3     
  M3_enc_act=M3_quad ;
  M3_enc_dif=M3_enc_ant-M3_enc_act ;
  M3_vel=(M3_enc_dif)*C2RS*RS2MS;
  M3_enc_ant=M3_enc_act ;       
    //Motor 4     
  M4_enc_act=M4_quad ;
  M4_enc_dif=M4_enc_ant-M4_enc_act ;
  M4_vel=(M4_enc_dif)*C2RS*RS2MS ;
  M4_enc_ant=M4_enc_act ;   
 
  if (Stop || millis()<1000){
    Ref_des=0;  
    Desv=0;
    M1_ref=0;
    M2_ref=0;
    M3_ref=0;
    M4_ref=0;
    ledcWrite(M1_CM_ch_dir, 0);
    ledcWrite(M1_CM_ch_inv, 0);
    ledcWrite(M2_CM_ch_dir, 0);
    ledcWrite(M2_CM_ch_inv, 0);
    ledcWrite(M3_CM_ch_dir, 0);
    ledcWrite(M3_CM_ch_inv, 0);
    ledcWrite(M4_CM_ch_dir, 0);
    ledcWrite(M4_CM_ch_inv, 0);
    /*Serial.print(acc) ; Serial.print(" d /// ") ;
    Serial.print(M2_ref) ; Serial.print(" r/s /// ") ;
    Serial.print(M2_vel) ; Serial.println(" ") ;//*/   
  }else{
    //Calculo de las referencias
    if(Desv<=0){
      Ref_der=Ref_des*(Desv);
    }else{
      Ref_der=Ref_des;
      }
    if(Desv<=0){
      Ref_iz=Ref_des*(Desv);
    }else{
      Ref_iz=Ref_des;
      }            
    //Ajuste de referencia lenta
    if (abs(M1_ref-Ref_der)<Cm_ref){
      M1_ref=Ref_der;
    }else{
     if (M1_ref < Ref_der){
      M1_ref=M1_ref+Cm_ref;
      }
      if (M1_ref > Ref_der){
        M1_ref=M1_ref-Cm_ref;
      }
    }
    if (abs(M2_ref-Ref_iz)<Cm_ref){
      M2_ref=Ref_iz;
    }else{
      if (M2_ref < Ref_iz){
      M2_ref=M2_ref+Cm_ref;
      }
      if (M2_ref > Ref_iz){
        M2_ref=M2_ref-Cm_ref;
      }
    }      
    if (abs(M3_ref-Ref_iz)<Cm_ref){
      M3_ref=Ref_iz;
    }else{
      if (M3_ref < Ref_iz){
        M3_ref=M3_ref+Cm_ref;
      }
      if (M3_ref > Ref_iz){
        M3_ref=M3_ref-Cm_ref;
      }
    }
    if (abs(M4_ref-Ref_der)<Cm_ref){
      M4_ref=Ref_der;
    }else{
      if (M4_ref < Ref_der){
        M4_ref=M4_ref+Cm_ref;
      }
      if (M4_ref > Ref_der){
        M4_ref=M4_ref-Cm_ref;
      }
    }    
   
    //Actualizacion PWM
    M1_Ik_1=Actualizamot( M1_ref,  M1_vel,  M1_Ik_1,M1_CM_ch_dir , M1_CM_ch_inv );
    /*Serial.print(acc) ; Serial.print(" d /// ") ;
    Serial.print(M1_ref) ; Serial.print(" r/s /// ") ;
    Serial.print(M1_vel) ; Serial.println(" ") ;// */
    M2_Ik_1=Actualizamot( M2_ref,  M2_vel,  M2_Ik_1,M2_CM_ch_dir , M2_CM_ch_inv );
    /*Serial.print(acc) ; Serial.print(" d /// ") ;
    Serial.print(M2_ref) ; Serial.print(" r/s /// ") ;
    Serial.print(M2_vel) ; Serial.println(" ") ;// */
   
    M3_Ik_1=Actualizamot( M3_ref,  M3_vel,  M3_Ik_1,M3_CM_ch_dir , M3_CM_ch_inv );
    /*Serial.print(acc) ; Serial.print(" d /// ") ;
    Serial.print(M3_ref) ; Serial.print(" r/s /// ") ;
    Serial.print(M3_vel) ; Serial.println(" ") ;// */
    M4_Ik_1=Actualizamot( M4_ref,  M4_vel,  M4_Ik_1,M4_CM_ch_dir , M4_CM_ch_inv ); 
    /*Serial.print(acc) ; Serial.print(" d /// ") ;
    Serial.print(M4_ref) ; Serial.print(" r/s /// ") ;
    Serial.print(M4_vel) ; Serial.println(" ") ;// */  

  /*Serial.print(Ref_der) ;Serial.print(" d /// ") ;
  Serial.print(acc) ; Serial.print(" d /// ") ;
  Serial.print(M4_ref) ; Serial.print(" r/s /// ") ;
  Serial.print(M4_vel) ; Serial.println(" ") ;// */

  /*Serial.print(M1_vel) ; Serial.print(" d /// ") ;
  Serial.print(M2_vel) ; Serial.print(" d /// ") ;
  Serial.print(M3_vel) ; Serial.print(" r/s /// ") ;
  Serial.print(M4_vel) ; Serial.println(" ") ; // */

  /*Serial.print(M1_ref) ; Serial.print(" d /// ") ;
  Serial.print(M2_ref) ; Serial.print(" d /// ") ;
  Serial.print(M3_ref) ; Serial.print(" r/s /// ") ;
  Serial.print(M4_ref) ; Serial.println(" ") ; // */
  //delay(50);
  //Ar=0;
  }    

  /*Serial.print("Parado: ") ;Serial.println(Stop);
  Serial.print("Velocidad: ") ;Serial.println(Ref_des);
  Serial.print("Proporción: ") ;Serial.println(Desv);
  Serial.print("Acceleración: ") ;Serial.println(Cm_ref);
  Serial.print("Linia: ") ;Serial.println(Btin);
  Serial.println("/////////////////////////////////////////////////////////////////");// */

  
  t2=millis() ;
  while (t2-t1<T) {t2=millis() ;}
  t1=millis() ;
}
