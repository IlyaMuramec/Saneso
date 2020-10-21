#include "stdafx.h"
#include "ScopeSettings.h"
#include "CameraSettings.h"
#include "LightCalSettings.h"
#include "json\json.h"

using namespace std;

log4cxx::LoggerPtr  CScopeSettings::logger(log4cxx::Logger::getLogger("ScopeSettings"));

CScopeSettings::CScopeSettings(std::string serialNumber, vector<CCameraSettings*> cameraSettings, CLightCalSettings *lightCalSettings)
{
	_ownsMemory = false;
	_cameraSettings = cameraSettings;
	_lightCalSettings = lightCalSettings;
}

CScopeSettings::CScopeSettings(std::string jsonEEProm)
{
	_ownsMemory = true;

	_lightCalSettings = new CLightCalSettings();

	Json::Reader reader;
	Json::Value root;
	bool parsingSuccessful = reader.parse(jsonEEProm, root);
	if (parsingSuccessful)
	{
		_cameraSettings.clear();

		const Json::Value& arraySettings = root["CameraSettings"];
		Json::ValueConstIterator itr = arraySettings.begin();
		int i = 0;
		for (; itr != arraySettings.end(); itr++)
		{
			CCameraSettings *settings = new CCameraSettings;
			settings->Load(*itr);
			_cameraSettings.push_back(settings);
		}

		_lightCalSettings->Load(root);
	}
	else
	{
		LOG4CXX_INFO(logger, "Unable to create Scope settings from EEPROM data");
	}
}

CScopeSettings::~CScopeSettings()
{
	if (_ownsMemory)
	{
		for (int i = 0; i < _cameraSettings.size(); i++)
		{
			delete _cameraSettings[i];
		}
		delete _lightCalSettings;
	}
}


std::string CScopeSettings::GetEEPromString()
{
	Json::Value root;
	Json::Value arraySettings;
	for (int i = 0; i < _cameraSettings.size(); i++)
	{
		Json::Value value;
		_cameraSettings[i]->Save(value);
		arraySettings.append(value);
	}
	root["CameraSettings"] = arraySettings;

	_lightCalSettings->Save(root);

	Json::FastWriter writer;
	return writer.write(root);
}

void CScopeSettings::UpdateFromEEPromString(std::string jsonEEProm)
{
	Json::Reader reader;
	Json::Value root;
	bool parsingSuccessful = reader.parse(jsonEEProm, root);
	if (parsingSuccessful)
	{
		const Json::Value& arraySettings = root["CameraSettings"];
		Json::ValueConstIterator itr = arraySettings.begin();
		
		if (arraySettings.size() == _cameraSettings.size())
		{
			int i = 0;
			for (; itr != arraySettings.end(); itr++, i++)
			{
				_cameraSettings[i]->Load(*itr);
			}
		}
		else
		{
			LOG4CXX_ERROR(logger, "EEProm has " << arraySettings.size() << " cameras intead of " << _cameraSettings.size());
		}

		_lightCalSettings->Load(root);
	}
	else
	{
		LOG4CXX_ERROR(logger, "Unable to update Scope settings from EEPROM data");
	}


}


