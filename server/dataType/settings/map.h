//
// Created by olivier on 02/05/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_SETTINGS_MAP_H
#define ONELIFE_SERVER_DATATYPE_SETTINGS_MAP_H

namespace OneLife::server::settings
{
	typedef struct
	{
		struct{

		}database;

		struct{
			SimpleVector<int> *biomeOrderList;
			SimpleVector<float> *biomeWeightList;
			SimpleVector<int> *specialBiomeList;
		}topography;
	}Map;
}

#endif //ONELIFE_SERVER_DATATYPE_SETTINGS_MAP_H
