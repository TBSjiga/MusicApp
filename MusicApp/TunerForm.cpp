#include "TunerForm.h"
#include <ctime>

using namespace System;
using namespace System::Windows::Forms;
using namespace NAudio::Wave;
using namespace NAudio;


void MusicApp::TunerForm::StartRecordMusic() {
	try {
		waveIn = gcnew WaveIn();

		//Получаем кол-во устройств захвата
		int waveInDevices = waveIn->DeviceCount;

		//Выводим все устройства захвата
		for (int i = 0; i < waveInDevices; i++) {

			WaveInCapabilities^ deviceInfo = waveIn->GetCapabilities(i);
			String^ deviceName = deviceInfo->ProductName;
			String^ device = "";
			devices[i] = deviceName;

			for (int j = 0; j < deviceName->Length; j++) {
				int o = deviceName->Length;
				if (j != 0 && j % 18 == 0) {
					device += deviceName[j].ToString();
					device += "\n";
				}
				else {
					device += deviceName[j].ToString();
				}
				
			}
			devices[i] = device;
		}

		
		if (comboBoxDevice->Items->Count == 0) {
			comboBoxDevice->Items->AddRange(devices);
		}
		
		waveIn->DeviceNumber = deviceNumSelected;

		//Получаем данные с микрофона в буфер
		waveIn->DataAvailable += gcnew EventHandler<WaveInEventArgs^>(this, &TunerForm::waveIn_DataAvailable);

		//Заканчиваем получать данные с устройства
		waveIn->RecordingStopped += gcnew EventHandler<StoppedEventArgs^>(this, &TunerForm::waveIn_RecordingStopped);

		//Устанавливаем параметры для выходного файла

		waveIn->WaveFormat = gcnew WaveFormat(44100, 16, 1);
		int blockAlign = (short)((1 * 16 + 7) / 8);
		int averageBytesPerSecond = blockAlign * 44100;
		WaveFormat waveFormat;
		WaveFormatEncoding newWaveFormat;
		waveIn->BufferMilliseconds = 250;
		waveIn->WaveFormat->CreateCustomFormat(newWaveFormat, 44100, 1, averageBytesPerSecond, blockAlign, 16);
		waveIn->WaveFormat->BlockAlign = (short)((waveIn->WaveFormat->Channels * waveIn->WaveFormat->BitsPerSample + 7) / 8);

		//Создаем файл с необходимыми параметрыми
		writer = gcnew WaveFileWriter(outputFileName, waveIn->WaveFormat);

		//Записываем в файл
		waveIn->StartRecording();
	}
	catch (Exception^ e) {
		MessageBox::Show(e->Message);
	}
}

//Получение данных из входного буффера
void MusicApp::TunerForm::waveIn_DataAvailable(System::Object^ sender, WaveInEventArgs^ e) {
	if (this->InvokeRequired) {
		this->BeginInvoke(gcnew EventHandler<WaveInEventArgs^>(this, &TunerForm::waveIn_DataAvailable), sender, e);
	}
	else {

		//переменная для обозначения начала распознавания речевого отрезка
		bool result = MusicApp::TunerForm::ProcessData(e);
		
		array<Byte>^ buf = gcnew array<Byte>(e->BytesRecorded);
		for (int i = 0; i < e->BytesRecorded; i++) {
			buf[i] = e->Buffer[i];
		}
		array<short>^ ar = gcnew array<short>(e->BytesRecorded);
		for (int i = 0; i < e->BytesRecorded; i++) {
			ar[i] = (short)e->Buffer[i];
		}

		array<double>^ arr = gcnew array<double>(e->BytesRecorded);
		for (int i = 0; i < e->BytesRecorded; i++) {
			arr[i] = ((short)buf[i]) / 32768.0;
		}



		double resultFreq = freqCheck.FindFundamentalFrequency(arr, 44100, 60, 440);
		labelHz->Text = resultFreq.ToString("f3");

		Записываем данные из буфера в файл
		writer->WriteData(e->Buffer, 0, e->BytesRecorded);

	}
}

//Окончание записи
void MusicApp::TunerForm::waveIn_RecordingStopped(System::Object^ sender, StoppedEventArgs^ e) {
	if (this->InvokeRequired) {
		this->BeginInvoke(gcnew EventHandler<StoppedEventArgs^>(this, &TunerForm::waveIn_RecordingStopped), sender, e);
	}
	else {
		waveIn->Dispose();

		waveIn = nullptr;
		writer->Close();
		writer = nullptr;

		saveAudioFile();
	}
}

//Завершаем запись
void MusicApp::TunerForm::StopRecording() {
	waveIn->StopRecording();
}

// Фильтр шума
bool MusicApp::TunerForm::ProcessData(WaveInEventArgs^ e)
{
	// Порог для вычисления наличия звука
	double porog = 0.02;
	bool result = false;
	bool Tr = false;
	double Sum2 = 0;
	int Count = e->BytesRecorded / 2;
	for (int index = 0; index < e->BytesRecorded; index += 2)
	{
		double Tmp = (short)((e->Buffer[index + 1] << 8) | e->Buffer[index + 0]);
		Tmp /= 32768.0;
		Sum2 += Tmp * Tmp;
		if (Tmp > porog)
			Tr = true;
	}
	Sum2 /= Count;
	if (Tr || Sum2 > porog)
	{
		result = true;
	}
	else
	{
		result = false;
	}
	return result;
}