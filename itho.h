#include "esphome.h"
#include "IthoCC1101.h"
#include "esp_task_wdt.h"


//List of States:

// 1 - Itho ventilation unit to lowest speed
// 2 - Itho ventilation unit to medium speed
// 3 - Itho ventilation unit to high speed
// 4 - Itho ventilation unit to full speed
// 13 -Itho to high speed with hardware timer (10 min)
// 23 -Itho to high speed with hardware timer (20 min)
// 33 -Itho to high speed with hardware timer (30 min)

// 100 - set Orcon ventilation unit to standby
// 101 - set Orcon ventilation unit to low speed
// 102 - set Orcon ventilation unit to medium speed
// 103 - set Orcon ventilation unit to high speed
// 104 - set Orcon ventilation unit to Auto
// 110 - set Orcon to standby with hardware timer (12 hours)
// 111 - set Orcon to low speed with hardware timer (1 hour)
// 112 - set Orcon to medium speed with hardware timer (13 hours)
// 113 - set Orcon to high speed with hardware timer (1 hour)
// 114 - set Orcon to AutoCO2 mode

typedef struct { String Id; String Roomname; } IdDict;

// Global struct to store Names, should be changed in boot call,to set user specific
IdDict Idlist[] = { {"ID1", "Controller Room1"},
					{"ID2",	"Controller Room2"},
					{"ID3",	"Controller Room3"}
				};
uint8_t srcID[3];
uint8_t destID[3];

IthoCC1101 *rf;
void ITHOinterrupt() IRAM_ATTR;
void ITHOcheck();

// extra for interrupt handling
bool ITHOhasPacket = false;

// init states
int State=1; // after startup it is assumed that the fan is running low
int OldState=1;
int Timer=0;

// init ID's
String LastID;
String OldLastID;
String Mydeviceid = "ESPHOME"; // should be changed in boot call,to set user specific

long LastPublish=0; 
bool InitRunned = false;

// Timer values for hardware timer in Fan
#define Time1      10*60
#define Time2      20*60
#define Time3      30*60

#define OrconTime0 (12*60*60)
#define OrconTime1 (60*60)
#define OrconTime2 (13*60*60)
#define OrconTime3 (60*60)

// TextSensor *InsReffanspeed; // Used for referencing outside FanRecv Class

String TextSensorfromState(int currentState)
{
	switch (currentState)
	{
        case 100:
                return "Standby";
                break;
	case 1: case 101:
		return "Low";
		break;
	case 2: case 102:
		return "Medium";
		break;
	case 3: case 103:
		return "High";
		break;
        case 104:
                return "Auto";
                break;
	case 13: case 23: case 33:
		return "High(T)";
		break;
	case 4: 
		return "Full";
		break;
	default:
	    return "Unknow";
		break;
	}
}


void setup_itho() {
    // ESP32: The CC1101 initialization involves multiple blocking SPI transfers
    // and calibration wait loops that can exceed the 5-second task watchdog timeout.
    // Temporarily remove this task from watchdog monitoring during init.
    esp_task_wdt_delete(NULL);

    rf = new IthoCC1101(5, 19);
    rf->enableOrcon(true);
    rf->init();
    // Followin wiring schema, change PIN if you wire differently
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), ITHOinterrupt, FALLING);
    rf->initReceive();
    InitRunned = true;

    // Re-add this task to watchdog monitoring now that init is complete
    esp_task_wdt_add(NULL);
}

void loop_itho() {
    if (ITHOhasPacket) {  // When Signal (from ISR) packet received, process packet 
        ITHOhasPacket=  false;
        ITHOcheck();
    }
}

void update_itho(TextSensor *fanspeed, TextSensor *fantimer, TextSensor *lastid) {
    if ((State >= 10) && (Timer > 0)) {
        Timer--;
        if (Timer == 0) { Timer--; }
    }

    if ((State >= 10) && (Timer < 0)) {
        if (State < 100) {
            State = 1;
        } else {
            State = 101;
        }
        Timer = 0;
        fantimer->publish_state(String(Timer).c_str()); // this ensures that times value 0 is published when elapsed
    }
    //Publish new data when vars are changed or timer is running
    if ((OldState != State) || (Timer > 0)|| InitRunned) {
        fanspeed->publish_state(TextSensorfromState(State).c_str());
        fantimer->publish_state(String(Timer).c_str());
        lastid->publish_state(LastID.c_str());
        OldState = State;
        InitRunned = false;
    }
}




void ITHOinterrupt()
{
	// Signal ITHO received  something
	ITHOhasPacket = true;
}


int RFRemoteIndex(String rfremoteid)
{
	if (rfremoteid == Idlist[0].Id) return 0;
	else if (rfremoteid == Idlist[1].Id) return 1;
	else if (rfremoteid == Idlist[2].Id) return 2;
	else return -1;
}


void ITHOcheck() {
  if (rf->checkForNewPacket()) {
    IthoCommand cmd = rf->getLastCommand();
	String Id = rf->getLastIDstr();
	
	// Print every received ID to the logs so the user can discover their remote!
	ESP_LOGI("custom", "Intercepted RF Packet from ID: %s", Id.c_str());
	
	int index = RFRemoteIndex(Id);
	if ( index>=0) { // Only accept commands that are in the list
		switch (cmd) {
		case IthoUnknown:
			ESP_LOGV("custom", "Unknown command");
			break;
		case IthoLow:
		case DucoLow:
			ESP_LOGD("custom", "IthoLow");
			State = 1;
			Timer = 0;
			LastID = Idlist[index].Roomname;
			break;
		case IthoMedium:
		case DucoMedium:
			ESP_LOGD("custom", "Medium");
			State = 2;
			Timer = 0;
			LastID = Idlist[index].Roomname;
			break;
		case IthoHigh:
		case DucoHigh:
			ESP_LOGD("custom", "High");
			State = 3;
			Timer = 0;
			LastID = Idlist[index].Roomname;
			break;
		case IthoFull:
			ESP_LOGD("custom", "Full");
			State = 4;
			Timer = 0;
			LastID = Idlist[index].Roomname;
			break;
		case IthoTimer1:
			ESP_LOGD("custom", "Timer1");
			State = 13;
			Timer = Time1;
			LastID = Idlist[index].Roomname;
			break;
		case IthoTimer2:
			ESP_LOGD("custom", "Timer2");
			State = 23;
			Timer = Time2;
			LastID = Idlist[index].Roomname;
			break;
		case IthoTimer3:
			ESP_LOGD("custom", "Timer3");
			State = 33;
			Timer = Time3;
			LastID = Idlist[index].Roomname;
			break;
		case IthoJoin:
			break;
		case IthoLeave:
			break;
                case OrconStandBy:
                        ESP_LOGD("custom", "Orcon Standby");
                        State = 100;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
			break;
                case OrconLow:
                        ESP_LOGD("custom", "Orcon Low");
                        State = 101;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconMedium:
                        ESP_LOGD("custom", "Orcon Medium");
                        State = 102;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconHigh:
                        ESP_LOGD("custom", "Orcon High");
                        State = 103;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconAuto:
                        ESP_LOGD("custom", "Orcon Auto");
                        State = 104;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconTimer0:
                        ESP_LOGD("custom", "Orcon Timer0");
                        State = 110;
                        Timer = OrconTime0;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconTimer1:
                        ESP_LOGD("custom", "Orcon Timer1");
                        State = 111;
                        Timer = OrconTime1;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconTimer2:
                        ESP_LOGD("custom", "Orcon Timer2");
                        State = 112;
                        Timer = OrconTime2;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconTimer3:
                        ESP_LOGD("custom", "Orcon Timer3");
                        State = 113;
                        Timer = OrconTime3;
                        LastID = Idlist[index].Roomname;
                        break;
                case OrconAutoCO2:
                        ESP_LOGD("custom", "Orcon Auto CO2");
                        State = 114;
                        Timer = 0;
                        LastID = Idlist[index].Roomname;
                        break;
		default:
		    break;
		}
	}
	else ESP_LOGV("","Ignored device-id: %s", Id.c_str());
  }
}
