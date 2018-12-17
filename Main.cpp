#include "monitor.h"
#include <stdio.h>
#include <queue>
#include <pthread.h>

class Queue_t : public Monitor
{
	public:
		Queue_t(char Id, unsigned int QueueLength) : Id(Id), QueueLength(QueueLength) {}
		
		void PutAlarm(unsigned int AlarmId)
		{
			enter();
			
			if(Alarms.size() == QueueLength)
				wait(NotFull);
			
			Alarms.push(AlarmId);
			if(Alarms.size() == 1)
				signal(NotEmpty);
			
			leave();
		}
		unsigned int GetAlarm()
		{
			enter();
			
			if(Alarms.size() == 0)
				wait(NotEmpty);
				
			
			unsigned int Alarm = Alarms.front();
			Alarms.pop();
			
			if(Alarms.size() == QueueLength - 1)
			{
				signal(NotFull);
			}
			
			leave();
			
			return Alarm;
		}
	protected:
		
		Condition NotFull;
		Condition NotEmpty;
		
		std::queue<unsigned int> Alarms;
		
		const char Id;
		const unsigned int QueueLength;
};
class QueueHigh_t : public Queue_t
{
	public:
		QueueHigh_t(char Id, unsigned int QueueLength): Queue_t(Id, QueueLength), ServiceCenterBusy(false) {}
		void PutAlarmWhileEmptyAndServiceCenterNotBusy(unsigned int AlarmId)
		{
			enter();
			
			if(Alarms.size() != 0 || ServiceCenterBusy)
				wait(EmptyAndServiceCenterNotBusy);
			
			Alarms.push(AlarmId);
			signal(NotEmpty);

			leave();
		}
		unsigned int GetAlarm()
		{
			enter();
			
			if(Alarms.size() == 0)
				wait(NotEmpty);
			
			unsigned int Alarm = Alarms.front();
			Alarms.pop();
			
			if(Alarms.size() == QueueLength - 1)
				signal(NotFull);
				
			ServiceCenterBusy = true;
			
			leave();
			
			return Alarm;
		}
		void OnServiceCenterUnBusy()
		{
			enter();
			
			ServiceCenterBusy = false;
			if(Alarms.size() == 0)
				signal(EmptyAndServiceCenterNotBusy);
			
			leave();
		}
	protected:
		bool ServiceCenterBusy;
		Condition EmptyAndServiceCenterNotBusy;
};

unsigned int LowProbes = 10;
unsigned int HighProbes = 1;
unsigned int LowAlarmsNo = 3;
unsigned int HighAlarmsNo = 3;
unsigned int QueueLength = 5;

Queue_t QueueLow('L', QueueLength);
QueueHigh_t QueueHigh('H', QueueLength);

void ProbeLow(unsigned int ProbeId)
{	
	for(unsigned int AlarmNo = 0; AlarmNo < LowAlarmsNo; AlarmNo++)
	{
		usleep(rand() % 500000);
		unsigned int Alarm = ProbeId * 1000 + AlarmNo;
		QueueLow.PutAlarm(Alarm);
		printf("[QueueLow		] Wygenerowano sygnal: %d\n", Alarm);
	}
}
void ProbeHigh(unsigned int ProbeId)
{	
	for(unsigned int AlarmNo = 0; AlarmNo < HighAlarmsNo; AlarmNo++)
	{
		usleep(rand() % 500000);
		unsigned int Alarm = ProbeId * 1000 + AlarmNo;
		QueueHigh.PutAlarm(Alarm);
		printf("[QueueHigh		] Wygenerowano sygnal: %d\n", Alarm);
	}
}
void Relay()
{
	for(unsigned int I = 0; I < LowProbes * LowAlarmsNo; I++)
	{
		usleep(rand() % 500000);
		auto Alarm = QueueLow.GetAlarm();
		QueueHigh.PutAlarmWhileEmptyAndServiceCenterNotBusy(Alarm);
		printf("[Relay			] Przeniesiono sygnal: %d\n", Alarm);
	}
}
void ServiceCenter()
{
	unsigned int AllAlarms = LowProbes * LowAlarmsNo + HighProbes * HighAlarmsNo;
	for(unsigned int I = 0; I < AllAlarms; I++)
	{	
		auto Alarm = QueueHigh.GetAlarm();
		
		usleep(rand() % 500000);
		printf("[ServiceCenter		] Odebrano alarm numer: %d (%d)\n", Alarm, I);
		
		QueueHigh.OnServiceCenterUnBusy();
	}
}


void * RelayP(void * Unused)
{
	Relay();
}
void * ServiceCenterP(void * Unused)
{
	ServiceCenter();
}
void * ProbeLowP(void * ProbeIdPtr)
{
	unsigned int ProbeId = * (unsigned int *) ProbeIdPtr; free(ProbeIdPtr);
	ProbeLow(ProbeId);
}
void * ProbeHighP(void * ProbeIdPtr)
{
	unsigned int ProbeId = * (unsigned int *) ProbeIdPtr; free(ProbeIdPtr);
	ProbeHigh(ProbeId);
}












int main(int ArgC, char ** ArgV)
{
	if(ArgC != 6)
	{
		printf("%s: <Liczba sond NP> <Liczba sond WP> <Liczba alarmow kazdej sondy NP> <Liczba alarmow kazdej sondy WP> <Dlugosc danych w kolejce>\n", ArgV[0]);
		return 1;
	}
	
	LowProbes = atoi(ArgV[1]);
	HighProbes = atoi(ArgV[2]);
	LowAlarmsNo = atoi(ArgV[3]);
	HighAlarmsNo = atoi(ArgV[4]);
	QueueLength = atoi(ArgV[5]);
	
	printf(
		"Uruchomienie: \n"
		" Liczba sond NP: %d\n"
		" Liczba sond WP: %d\n"
		" Liczba alarmow kazdej sondy NP: %d\n"
		" Liczba alarmow kazdej sondy WP: %d\n"
		" Dlugosc danych w kolejce: %d\n"
		"\n=======================================\n\n"
		,
		LowProbes, HighProbes, LowAlarmsNo, HighAlarmsNo, QueueLength
	);
	
	
	pthread_t RelayT, ServiceCenterT;
	pthread_t * ProbeLowT = new pthread_t[LowProbes];
	pthread_t * ProbeHighT = new pthread_t[HighProbes];
	
	pthread_create(&RelayT, NULL, RelayP, (void *) nullptr);
	pthread_create(&ServiceCenterT, NULL, ServiceCenterP, (void *) nullptr);
	for(unsigned int I = 0; I < LowProbes; I++)
		pthread_create(&ProbeLowT[I], NULL, ProbeLowP, (void *) new unsigned int((I + 1) * 1));
	for(unsigned int I = 0; I < HighProbes; I++)
		pthread_create(&ProbeHighT[I], NULL, ProbeHighP, (void *) new unsigned int((I + 1) * 1000));
	
	pthread_join(RelayT, NULL);
	pthread_join(ServiceCenterT, NULL);
	for(unsigned int I = 0; I < LowProbes; I++)
		pthread_join(ProbeLowT[I], NULL);
	for(unsigned int I = 0; I < HighProbes; I++)
		pthread_join(ProbeHighT[I], NULL);
	
	return 0;
}
