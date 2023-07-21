#pragma once
#include "src/Core/Core.h"

namespace Nuake
{
	class Prefab;
	class PrefabComponent
	{
	public:
		Ref<Prefab> PrefabInstance;

		void SetPrefab(Ref<Prefab> prefab)
		{
			PrefabInstance = prefab;
		}
	};
}