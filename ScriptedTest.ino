/* 
This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
It won't work with v1.x motor shields! Only for the v2's with built in PWM
control

For use with the Adafruit Motor Shield v2 
---->	http://www.adafruit.com/products/1438
*/


#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"

#include <TinyXML.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);

// XMLDocument this is a reference to the document that is loaded 
// from an SDCard which defines the test parameters.
TinyXML xml;

const char document[] = {
"<?xml version='1.0' encoding='UTF-8'?>		\
<test name='stepper motor test'>		\
  <stepper motion='single' distance='100' direction='forward'/>		\
  <stepper motion='single' distance='100' direction='backward'/>		\
  <pause seconds='10'/>		\
  <stepper motion='double' distance='100' direction='forward'/>		\
  <stepper motion='double' distance='100' direction='backward'/>		\
  <pause seconds='15'/>		\
  <stepper motion='interleave' distance='150' direction='forward'/>		\
  <stepper motion='interleave' distance='150' direction='backward'/>		\
  <pause seconds='20'/>		\
  <stepper motion='microstep' distance='100' direction='forward'/>		\
  <stepper motion='microstep' distance='100' direction='backward'/>		\
</test>"
};

uint8_t buffer[1024] = {0};

typedef struct {
	uint16_t steps;
	uint8_t dir;
	uint8_t style;
} motor_test_t;

const motor_test_t motor_test_default = {
	.steps = 50,
	.dir = FORWARD,
	.style = SINGLE,
};

typedef enum {
	UNKNOWN_TEST,
	MOTOR_TEST,
	PAUSE_TEST
} current_test_t;

static motor_test_t motor_test;
static uint8_t pause_time = 0;
static current_test_t current_test = UNKNOWN_TEST;
static bool valid_file = true;
static uint16_t doc_size = 0;
static uint8_t load_timeout = LOAD_TIMEOUT;

bool tagcmp(const char *t1, const char *t2)
{
	int t1_s = strlen(t1);
	int t2_s = strlen(t2);

	if (t2_s >= t1_s)
	{
		t1_s--;
		t2_s--;

		while (t1[t1_s] == t2[t2_s])
		{
			if (t1_s)
			{
				t1_s--;
				t2_s--;
			}
			else
			{
				/* if of equal length or t2 is path seperator */
				if ((0 == t1_s) &&
					((0 == t2_s) ||
					 ('/' == t2[t2_s -1])))
				{
					return true;
				}
			}
		}
	}
	return false;
}

void XML_callback( uint8_t statusflags, char* tagName,  uint16_t tagNameLen,  char* data,  uint16_t dataLen )
{
  if (statusflags & STATUS_START_TAG)
  {
    if ( tagNameLen )
    {

		if (tagcmp("stepper", tagName))
		{
			motor_test = motor_test_default;
			current_test = MOTOR_TEST;
		}
		else
		if (tagcmp("pause", tagName))
		{
			current_test = PAUSE_TEST;
		}
		else
		{
			current_test = UNKNOWN_TEST;
		}
    }
  }
  else if (statusflags & STATUS_END_TAG)
  {
	  switch (current_test)
	  {
		  case MOTOR_TEST:
			  if (tagcmp("stepper", tagName))
			  {
				  myMotor->step(motor_test.steps, motor_test.dir, motor_test.style);
			  }
			  break;
		  case PAUSE_TEST:
			  if (tagcmp("pause", tagName))
			  {
				  delay( 1000 * (uint32_t)pause_time );
			  }
			  break;
		  default:
			  break;
	  }
  }
  else if  (statusflags & STATUS_TAG_TEXT)
  {
  }
  else if  (statusflags & STATUS_ATTR_TEXT)
  {
	  /* use test specific parsing */
	  if (dataLen)
	  {
		  if (0 == strcmp( "motion", tagName))
		  {
			  if (0 == strcmp("microstep", data))
			  {
				  motor_test.style = MICROSTEP;
			  }
			  else
			  if (0 == strcmp("single", data))
			  {
				  motor_test.style = SINGLE;
			  }
			  else
			  if (0 == strcmp("double", data))
			  {
				  motor_test.style = DOUBLE;
			  }
			  else
			  if (0 == strcmp("interleave", data))
			  {
				  motor_test.style = INTERLEAVE;
			  }
		  }
		  else if (0 == strcmp( "direction", tagName ))
		  {
			  if (0 == strcmp( "forward", data ))
			  {
				  motor_test.dir = FORWARD;
			  }
			  else if (0 == strcmp( "backward", data ))
			  {
				  motor_test.dir = BACKWARD;
			  }
			  else if (0 == strcmp( "brake", data ))
			  {
				  motor_test.dir = BRAKE;
			  }
			  else if (0 == strcmp( "release", data ))
			  {
				  motor_test.dir = RELEASE;
			  }
		  }
		  else if (0 == strcmp( "distance", tagName ))
		  {
			  motor_test.steps = atoi( data );
		  }
		  else if (0 == strcmp( "seconds", tagName ))
		  {
			  pause_time = atoi( data );
		  }
	  }/* dataLen */
  }
  else if  (statusflags & STATUS_ERROR)
  {
    Serial.print("XML Parsing error  Tag:");
    Serial.print(tagName);
    Serial.print(" text:");
    Serial.println(data);
	
	valid_file = false;
	doc_size = 0;
  }
}

/*********************************************************************************/
/* non static functions */
/*********************************************************************************/

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Stepper test!");
  uint16_t buflen = 1024;
  doc_size = strlen(document);

  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  // setup xml stuff
  xml.init((uint8_t*)&buffer,buflen,&XML_callback);
  
  myMotor->setSpeed(10);  // 10 rpm   
}

void loop()
{
		unsigned int i = 0;

		for (i=0; i< strlen(document); i++)
		{
			xml.processChar(document[i]);
		}

}

