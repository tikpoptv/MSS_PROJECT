void get_Automatic_PIR(){
  Serial.println("get_Automatic_PIR_ONLINE");
  if(timeClient.isTimeSet()) {
    if (PIR_START_hour == timeClient.getHours() && PIR_START_minute == timeClient.getMinutes()) {
           
                   
  
    } else if (PIR_START_hour_PM <= timeClient.getHours() || PIR_START_hour_AM >= timeClient.getHours())  {
            Automatic_PIR_Status = 1;            
            pir_on_once = true;   
            Serial.println("เริ่มทำงานอัตโนมัติ");    
        }
    if (PIR_END_hour == timeClient.getHours() && PIR_END_minute == timeClient.getMinutes()) {

        Serial.println("หยุดการทำงานอัตโนมัติ");      
        LINE.notify("หยุดการทำงานอัตโนมัติ");                    
        Automatic_PIR_Status = 0; 

    }
  }
}