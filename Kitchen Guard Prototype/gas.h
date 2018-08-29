/*
 * ========= Gas sensor reading and PPM conversions=========
 */
 
#define AN_SENSOR_0         35
#define AN_READ_INTERVAL    3
#define AN_MAX_LEVEL        4096
#define AN_VREF             3.30

#define MQ_RSR0_BASE_RATIO  6.5     
#define MQ_R0_LPG           910
#define MQ_COEFF_A          76.334
#define MQ_COEFF_B         -2.5070

double  MQ_RS_R0    =       0.0000;
double  MQ_LPG_PPM  =       0.0000;
double  MQ_LPG_LTH  =       20.000;
double  MQ_LPG_ATH  =       50.000;
double  MQ_LPG_CTH  =       100.00;


int getMQRawData(int channel)
{
  uint8_t readCount         = 20;
  uint32_t rawAnalogReading = 0;
  for(uint8_t i=0; i<readCount; i++)
  {
    rawAnalogReading += analogRead(channel);
    delay(AN_READ_INTERVAL);
  }
  rawAnalogReading = rawAnalogReading/(uint32_t)readCount;
  return rawAnalogReading;
}

double getMQRsOhm(int rawAdcValue)
{
  double Rs  = 0.00;
  double Rl  = 1000;
  double Vs  = 0.00;

  Vs = (double)AN_VREF - ((rawAdcValue*(double)AN_VREF)/(double)AN_MAX_LEVEL);
  Rs = (Rl*Vs)/((double)AN_VREF-Vs);
  MQ_RS_R0 = Rs/MQ_R0_LPG;
  return Rs;
}

double getMQPPM(double Rs )
{
  double Rs_R0_Ratio = Rs/MQ_R0_LPG;
  double PPM = (double)MQ_COEFF_A * (pow(Rs_R0_Ratio, MQ_COEFF_B));
  return PPM;
}

double getGasLevel(void)
{
  double x = getMQRawData(AN_SENSOR_0);
  double y = getMQRsOhm(x);  
  double z = getMQPPM(y);
  return z;
}

double getAvgGasLevel(int smoothCounter=3)
{
  double gasLevel = 0.0000; 
  for(int i=0; i<smoothCounter; i++)
  {
    gasLevel += getGasLevel();
  }
  gasLevel = gasLevel/(double)smoothCounter;
  MQ_LPG_PPM = gasLevel;
  return gasLevel;
}



