/*-------------------------------------------------------------------------------

	BARONY
	File: actgeneral.cpp
	Desc: very small and general behavior functions

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "sound.hpp"
#include "entity.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "player.hpp"

/*-------------------------------------------------------------------------------

	act*

	The following functions describe various entity behaviors. All functions
	take a pointer to the entities that use them as an argument.

-------------------------------------------------------------------------------*/

void actAnimator(Entity* my)
{
	if ( my->skill[4] == 0 )
	{
		my->skill[4] = 1;
		map.tiles[my->skill[0] + (int)my->y * MAPLAYERS + (int)my->x * MAPLAYERS * map.height] -= my->skill[1] - 1;
	}

	if ( (int)floor(my->x) < 0 || (int)floor(my->x) >= map.width || (int)floor(my->y) < 0 || (int)floor(my->y) >= map.height )
	{
		list_RemoveNode(my->mynode);
		return;
	}

	my->skill[3]++;
	if ( my->skill[3] >= 10 )
	{
		my->skill[3] = 0;
		map.tiles[my->skill[0] + (int)floor(my->y)*MAPLAYERS + (int)floor(my->x)*MAPLAYERS * map.height]++;
		my->skill[5]++;
		if (my->skill[5] == my->skill[1])
		{
			my->skill[5] = 0;
			map.tiles[my->skill[0] + (int)floor(my->y)*MAPLAYERS + (int)floor(my->x)*MAPLAYERS * map.height] -= my->skill[1];
		}
	}
}

#define TESTSPRITES

void actRotate(Entity* my)
{
	my->yaw += 0.1;
	my->flags[PASSABLE] = true; // this entity should always be passable

#ifdef TESTSPRITES
	if ( keystatus[SDL_SCANCODE_HOME] )
	{
		keystatus[SDL_SCANCODE_HOME] = 0;
		my->sprite++;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, "test sprite: %d", my->sprite);
	}
	if ( keystatus[SDL_SCANCODE_END] )
	{
		keystatus[SDL_SCANCODE_END] = 0;
		my->sprite += 10;
		if ( my->sprite >= nummodels )
		{
			my->sprite = 0;
		}
		messagePlayer(clientnum, "test sprite: %d", my->sprite);
	}
#endif
}

#define LIQUID_TIMER my->skill[0]
#define LIQUID_INIT my->skill[1]
#define LIQUID_LAVA my->flags[USERFLAG1]
#define LIQUID_LAVANOBUBBLE my->skill[4]

void actLiquid(Entity* my)
{
	// as of 1.0.7 this function is DEPRECATED

	list_RemoveNode(my->mynode);
	return;

	if ( !LIQUID_INIT )
	{
		LIQUID_INIT = 1;
		LIQUID_TIMER = 60 * (rand() % 20);
		if ( LIQUID_LAVA )
		{
			my->light = lightSphereShadow(my->x / 16, my->y / 16, 2, 128);
		}
	}
	LIQUID_TIMER--;
	if ( LIQUID_TIMER <= 0 )
	{
		LIQUID_TIMER = 60 * 20 + 60 * (rand() % 20);
		if ( !LIQUID_LAVA )
		{
			playSoundEntityLocal( my, 135, 32 );
		}
		else
		{
			playSoundEntityLocal( my, 155, 100 );
		}
	}
	if ( LIQUID_LAVA && !LIQUID_LAVANOBUBBLE )
	{
		if ( ticks % 40 == my->getUID() % 40 && rand() % 3 == 0 )
		{
			int c, j = 1 + rand() % 2;
			for ( c = 0; c < j; c++ )
			{
				Entity* entity = spawnGib( my );
				entity->x += rand() % 16 - 8;
				entity->y += rand() % 16 - 8;
				entity->flags[SPRITE] = true;
				entity->sprite = 42;
				entity->fskill[3] = 0.01;
				double vel = (rand() % 10) / 20.f;
				entity->vel_x = vel * cos(entity->yaw);
				entity->vel_y = vel * sin(entity->yaw);
				entity->vel_z = -.15 - (rand() % 15) / 100.f;
				entity->z = 7.5;
			}
		}
	}
}

void actEmpty(Entity* my)
{
	// an empty action
	// used on clients to permit dead reckoning and other interpolation
}

void actFurniture(Entity* my)
{
	if ( !my )
	{
		return;
	}

	my->actFurniture();
}

void Entity::actFurniture()
{
	if ( !furnitureInit )
	{
		furnitureInit = 1;
		if ( !furnitureType )
		{
			furnitureHealth = 15 + rand() % 5;
		}
		else
		{
			furnitureHealth = 4 + rand() % 4;
		}
		furnitureMaxHealth = furnitureHealth;
		flags[BURNABLE] = true;
	}
	else
	{
		if ( multiplayer != CLIENT )
		{
			// burning
			if ( flags[BURNING] )
			{
				if ( ticks % 15 == 0 )
				{
					furnitureHealth--;
				}
			}

			// furniture mortality :p
			if ( furnitureHealth <= 0 )
			{
				int c;
				for ( c = 0; c < 5; c++ )
				{
					Entity* entity = spawnGib(this);
					entity->flags[INVISIBLE] = false;
					entity->sprite = 187; // Splinter.vox
					entity->x = floor(x / 16) * 16 + 8;
					entity->y = floor(y / 16) * 16 + 8;
					entity->y += -3 + rand() % 6;
					entity->x += -3 + rand() % 6;
					entity->z = -5 + rand() % 10;
					entity->yaw = (rand() % 360) * PI / 180.0;
					entity->pitch = (rand() % 360) * PI / 180.0;
					entity->roll = (rand() % 360) * PI / 180.0;
					entity->vel_x = (rand() % 10 - 5) / 10.0;
					entity->vel_y = (rand() % 10 - 5) / 10.0;
					entity->vel_z = -.5;
					entity->fskill[3] = 0.04;
					serverSpawnGibForClient(entity);
				}
				playSoundEntity(this, 176, 128);
				Entity* entity;
				if ( (entity = uidToEntity(parent)) != NULL )
				{
					entity->skill[18] = 0; // drop the item that was on the table
					serverUpdateEntitySkill(entity, 18);
				}
				list_RemoveNode(mynode);
				return;
			}

			// using
			int i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( (i == 0 && selectedEntity == this) || (client_selected[i] == this) )
				{
					if (inrange[i])
					{
						if ( furnitureType )
						{
							messagePlayer(i, language[476]);
						}
						else
						{
							messagePlayer(i, language[477]);
						}
					}
				}
			}
		}
	}
}

// an easter egg
#define MCAXE_USED my->skill[0]

void actMCaxe(Entity* my)
{
	my->yaw += .05;
	if ( my->yaw > PI * 2 )
	{
		my->yaw -= PI * 2;
	}
	if ( !MCAXE_USED )
	{
		if ( multiplayer != CLIENT )
		{
			// use
			int i;
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if ( (i == 0 && selectedEntity == my) || (client_selected[i] == my) )
				{
					if (inrange[i])
					{
						messagePlayer(i, language[478 + rand() % 5]);
						MCAXE_USED = 1;
						serverUpdateEntitySkill(my, 0);
					}
				}
			}
		}

		// bob
		my->z -= sin(my->fskill[0] * PI / 180.f);
		my->fskill[0] += 6;
		if ( my->fskill[0] >= 360 )
		{
			my->fskill[0] -= 360;
		}
		my->z += sin(my->fskill[0] * PI / 180.f);
	}
	else
	{
		my->z += 1;
		if ( my->z > 64 )
		{
			list_RemoveNode(my->mynode);
			return;
		}
	}
}

void actStalagFloor(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}

	//my->actStalagFloor();
}

void actStalagCeiling(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}

	//my->actStalagCeiling();
}

void actStalagColumn(Entity* my)
{
	//TODO: something?
	if ( !my )
	{
		return;
	}

	//my->actStalagColumn();
}

void actCeilingTile(Entity* my)
{
	if ( !my )
	{
		return;
	}
}