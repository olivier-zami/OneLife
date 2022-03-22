//
// Created by olivier on 21/03/2022.
//

this->socket->readMessage(message);

int sizeX = 0;
int sizeY = 0;
int x     = 0;
int y     = 0;

int binarySize     = 0;
int compressedSize = 0;

sscanf(message, "MC\n%d %d %d %d\n%d %d\n", &sizeX, &sizeY, &x, &y, &binarySize, &compressedSize);

printf("Got map chunk with bin size %d, compressed size %d\n", binarySize, compressedSize);

if (!mMapGlobalOffsetSet)
{

	// we need 7 fraction bits to represent 128 pixels per tile
	// 32-bit float has 23 significant bits, not counting sign
	// that leaves 16 bits for tile coordinate, or 65,536
	// Give two extra bits of wiggle room
	int maxOK = 16384;

	if (x < maxOK && x > -maxOK && y < maxOK && y > -maxOK)
	{
		printf("First chunk isn't too far from center, using "
			   "0,0 as our global offset\n");

		mMapGlobalOffset.x  = 0;
		mMapGlobalOffset.y  = 0;
		mMapGlobalOffsetSet = true;
	}
	else
	{
		printf("Using this first chunk center as our global offset:  "
			   "%d, %d\n",
			x,
			y);
		mMapGlobalOffset.x  = x;
		mMapGlobalOffset.y  = y;
		mMapGlobalOffsetSet = true;
	}
}

applyReceiveOffset(&x, &y);

// recenter our in-ram sub-map around this new chunk
int newMapOffsetX = x + sizeX / 2;
int newMapOffsetY = y + sizeY / 2;

// move old cached map cells over to line up with new center

int xMove = mMapOffsetX - newMapOffsetX;
int yMove = mMapOffsetY - newMapOffsetY;

int *newMap       = new int[mMapD * mMapD];
int *newMapBiomes = new int[mMapD * mMapD];
int *newMapFloors = new int[mMapD * mMapD];

double *newMapAnimationFrameCount     = new double[mMapD * mMapD];
double *newMapAnimationLastFrameCount = new double[mMapD * mMapD];

double *newMapAnimationFrozenRotFameCount = new double[mMapD * mMapD];

char *newMapAnimationFrozenRotFameCountUsed = new char[mMapD * mMapD];

int *newMapFloorAnimationFrameCount = new int[mMapD * mMapD];

AnimType *newMapCurAnimType  = new AnimType[mMapD * mMapD];
AnimType *newMapLastAnimType = new AnimType[mMapD * mMapD];
double *  newMapLastAnimFade = new double[mMapD * mMapD];

doublePair *newMapDropOffsets = new doublePair[mMapD * mMapD];
double *    newMapDropRot     = new double[mMapD * mMapD];
SoundUsage *newMapDropSounds  = new SoundUsage[mMapD * mMapD];

doublePair *newMapMoveOffsets = new doublePair[mMapD * mMapD];
double *    newMapMoveSpeeds  = new double[mMapD * mMapD];

char *newMapTileFlips = new char[mMapD * mMapD];

SimpleVector<int> *newMapContainedStacks = new SimpleVector<int>[mMapD * mMapD];

SimpleVector<SimpleVector<int>> *newMapSubContainedStacks = new SimpleVector<SimpleVector<int>>[mMapD * mMapD];

char *newMapPlayerPlacedFlags = new char[mMapD * mMapD];

for (int i = 0; i < mMapD * mMapD; i++)
{
	// starts uknown, not empty
	newMap[i]       = -1;
	newMapBiomes[i] = -1;
	newMapFloors[i] = -1;

	int newX = i % mMapD;
	int newY = i / mMapD;

	int worldX = newX + mMapOffsetX - mMapD / 2;
	int worldY = newY + mMapOffsetY - mMapD / 2;

	// each cell is different, but always the same
	newMapAnimationFrameCount[i]     = lrint(getXYRandom(worldX, worldY) * 10000);
	newMapAnimationLastFrameCount[i] = newMapAnimationFrameCount[i];

	newMapAnimationFrozenRotFameCount[i]     = 0;
	newMapAnimationFrozenRotFameCountUsed[i] = false;

	newMapFloorAnimationFrameCount[i] = lrint(getXYRandom(worldX, worldY) * 13853);

	newMapCurAnimType[i]   = ground;
	newMapLastAnimType[i]  = ground;
	newMapLastAnimFade[i]  = 0;
	newMapDropOffsets[i].x = 0;
	newMapDropOffsets[i].y = 0;
	newMapDropRot[i]       = 0;

	newMapDropSounds[i] = blankSoundUsage;

	newMapMoveOffsets[i].x = 0;
	newMapMoveOffsets[i].y = 0;
	newMapMoveSpeeds[i]    = 0;

	newMapTileFlips[i]         = false;
	newMapPlayerPlacedFlags[i] = false;

	int oldX = newX - xMove;
	int oldY = newY - yMove;

	if (oldX >= 0 && oldX < mMapD && oldY >= 0 && oldY < mMapD)
	{

		int oI = oldY * mMapD + oldX;

		newMap[i]       = mMap[oI];
		newMapBiomes[i] = mMapBiomes[oI];
		newMapFloors[i] = mMapFloors[oI];

		newMapAnimationFrameCount[i]     = mMapAnimationFrameCount[oI];
		newMapAnimationLastFrameCount[i] = mMapAnimationLastFrameCount[oI];

		newMapAnimationFrozenRotFameCount[i] = mMapAnimationFrozenRotFrameCount[oI];

		newMapAnimationFrozenRotFameCountUsed[i] = mMapAnimationFrozenRotFrameCountUsed[oI];

		newMapFloorAnimationFrameCount[i] = mMapFloorAnimationFrameCount[oI];

		newMapCurAnimType[i]  = mMapCurAnimType[oI];
		newMapLastAnimType[i] = mMapLastAnimType[oI];
		newMapLastAnimFade[i] = mMapLastAnimFade[oI];
		newMapDropOffsets[i]  = mMapDropOffsets[oI];
		newMapDropRot[i]      = mMapDropRot[oI];
		newMapDropSounds[i]   = mMapDropSounds[oI];

		newMapMoveOffsets[i] = mMapMoveOffsets[oI];
		newMapMoveSpeeds[i]  = mMapMoveSpeeds[oI];

		newMapTileFlips[i] = mMapTileFlips[oI];

		newMapContainedStacks[i]    = mMapContainedStacks[oI];
		newMapSubContainedStacks[i] = mMapSubContainedStacks[oI];

		newMapPlayerPlacedFlags[i] = mMapPlayerPlacedFlags[oI];
	}
}

memcpy(mMap, newMap, mMapD *mMapD * sizeof(int));
memcpy(mMapBiomes, newMapBiomes, mMapD *mMapD * sizeof(int));
memcpy(mMapFloors, newMapFloors, mMapD *mMapD * sizeof(int));

memcpy(mMapAnimationFrameCount, newMapAnimationFrameCount, mMapD *mMapD * sizeof(double));
memcpy(mMapAnimationLastFrameCount, newMapAnimationLastFrameCount, mMapD *mMapD * sizeof(double));

memcpy(mMapAnimationFrozenRotFrameCount, newMapAnimationFrozenRotFameCount, mMapD *mMapD * sizeof(double));

memcpy(mMapAnimationFrozenRotFrameCountUsed, newMapAnimationFrozenRotFameCountUsed, mMapD *mMapD * sizeof(char));

memcpy(mMapFloorAnimationFrameCount, newMapFloorAnimationFrameCount, mMapD *mMapD * sizeof(int));

memcpy(mMapCurAnimType, newMapCurAnimType, mMapD *mMapD * sizeof(AnimType));
memcpy(mMapLastAnimType, newMapLastAnimType, mMapD *mMapD * sizeof(AnimType));
memcpy(mMapLastAnimFade, newMapLastAnimFade, mMapD *mMapD * sizeof(double));
memcpy(mMapDropOffsets, newMapDropOffsets, mMapD *mMapD * sizeof(doublePair));
memcpy(mMapDropRot, newMapDropRot, mMapD *mMapD * sizeof(double));
memcpy(mMapDropSounds, newMapDropSounds, mMapD *mMapD * sizeof(SoundUsage));

memcpy(mMapMoveOffsets, newMapMoveOffsets, mMapD *mMapD * sizeof(doublePair));
memcpy(mMapMoveSpeeds, newMapMoveSpeeds, mMapD *mMapD * sizeof(double));

memcpy(mMapTileFlips, newMapTileFlips, mMapD *mMapD * sizeof(char));

// can't memcpy vectors
// need to assign them so copy constructors are invoked
for (int i = 0; i < mMapD * mMapD; i++)
{
	mMapContainedStacks[i]    = newMapContainedStacks[i];
	mMapSubContainedStacks[i] = newMapSubContainedStacks[i];
}

memcpy(mMapPlayerPlacedFlags, newMapPlayerPlacedFlags, mMapD *mMapD * sizeof(char));

delete[] newMap;
delete[] newMapBiomes;
delete[] newMapFloors;
delete[] newMapAnimationFrameCount;
delete[] newMapAnimationLastFrameCount;
delete[] newMapAnimationFrozenRotFameCount;
delete[] newMapAnimationFrozenRotFameCountUsed;
delete[] newMapFloorAnimationFrameCount;

delete[] newMapCurAnimType;
delete[] newMapLastAnimType;
delete[] newMapLastAnimFade;
delete[] newMapDropOffsets;
delete[] newMapDropRot;
delete[] newMapDropSounds;

delete[] newMapMoveOffsets;
delete[] newMapMoveSpeeds;

delete[] newMapTileFlips;
delete[] newMapContainedStacks;
delete[] newMapSubContainedStacks;

delete[] newMapPlayerPlacedFlags;

mMapOffsetX = newMapOffsetX;
mMapOffsetY = newMapOffsetY;

unsigned char *compressedChunk = new unsigned char[compressedSize];

for (int i = 0; i < compressedSize; i++)
{
	compressedChunk[i] = serverSocketBuffer.getElementDirect(i);
}
serverSocketBuffer.deleteStartElements(compressedSize);

unsigned char *decompressedChunk = zipDecompress(compressedChunk, compressedSize, binarySize);

delete[] compressedChunk;

if (decompressedChunk == NULL) { printf("Decompressing chunk failed\n"); }
else
{

	unsigned char *binaryChunk = new unsigned char[binarySize + 1];

	memcpy(binaryChunk, decompressedChunk, binarySize);

	delete[] decompressedChunk;

	// for now, binary chunk is actually just ASCII
	binaryChunk[binarySize] = '\0';

	SimpleVector<char *> *tokens = tokenizeString((char *)binaryChunk);

	delete[] binaryChunk;

	int numCells = sizeX * sizeY;

	if (tokens->size() == numCells)
	{

		for (int i = 0; i < tokens->size(); i++)
		{
			int cX = i % sizeX;
			int cY = i / sizeX;

			int mapX = cX + x - mMapOffsetX + mMapD / 2;
			int mapY = cY + y - mMapOffsetY + mMapD / 2;

			if (mapX >= 0 && mapX < mMapD && mapY >= 0 && mapY < mMapD)
			{

				int mapI     = mapY * mMapD + mapX;
				int oldMapID = mMap[mapI];

				sscanf(
					tokens->getElementDirect(i), "%d:%d:%d", &(mMapBiomes[mapI]), &(mMapFloors[mapI]), &(mMap[mapI]));

				if (mMap[mapI] != oldMapID)
				{
					// our placement status cleared
					mMapPlayerPlacedFlags[mapI] = false;
				}

				mMapContainedStacks[mapI].deleteAll();
				mMapSubContainedStacks[mapI].deleteAll();

				if (strstr(tokens->getElementDirect(i), ",") != NULL)
				{

					int    numInts;
					char **ints = split(tokens->getElementDirect(i), ",", &numInts);

					delete[] ints[0];

					int numContained = numInts - 1;

					for (int c = 0; c < numContained; c++)
					{
						SimpleVector<int> newSubStack;

						mMapSubContainedStacks[mapI].push_back(newSubStack);

						int contained = atoi(ints[c + 1]);
						mMapContainedStacks[mapI].push_back(contained);

						if (strstr(ints[c + 1], ":") != NULL)
						{
							// sub-container items

							int    numSubInts;
							char **subInts = split(ints[c + 1], ":", &numSubInts);

							delete[] subInts[0];
							int numSubCont = numSubInts - 1;

							SimpleVector<int> *subStack = mMapSubContainedStacks[mapI].getElement(c);

							for (int s = 0; s < numSubCont; s++)
							{
								subStack->push_back(atoi(subInts[s + 1]));
								delete[] subInts[s + 1];
							}

							delete[] subInts;
						}

						delete[] ints[c + 1];
					}
					delete[] ints;
				}
			}
		}
	}

	tokens->deallocateStringElements();
	delete tokens;

	if (!(mFirstServerMessagesReceived & 1))
	{
		// first map chunk just recieved

		minitech::initOnBirth();

		// reset fov on birth
		if (SettingsManager::getIntSetting("fovEnabled", 1))
		{ changeFOV(SettingsManager::getFloatSetting("fovDefault", 1.25f)); }
		else
		{
			changeFOV(1.0f);
		}
		changeHUDFOV(SettingsManager::getFloatSetting("fovScaleHUD", 1.25f));

		char found    = false;
		int  closestX = 0;
		int  closestY = 0;

		// only if marker starts on birth map chunk

		// use distance squared here, no need for sqrt

		// rough estimate of radius of birth map chunk
		// this allows markers way off the screen, but so what?
		double closestDist = 16 * 16;

		int mapCenterY = y + sizeY / 2;
		int mapCenterX = x + sizeX / 2;
		printf("Map center = %d,%d\n", mapCenterX, mapCenterY);

		for (int mapY = 0; mapY < mMapD; mapY++)
		{
			for (int mapX = 0; mapX < mMapD; mapX++)
			{

				int i = mMapD * mapY + mapX;

				int id = mMap[i];

				if (id > 0)
				{

					// check for home marker
					if (getObject(id)->homeMarker)
					{
						int worldY = mapY + mMapOffsetY - mMapD / 2;

						int worldX = mapX + mMapOffsetX - mMapD / 2;

						double dist = pow(worldY - mapCenterY, 2) + pow(worldX - mapCenterX, 2);

						if (dist < closestDist)
						{
							closestDist = dist;
							closestX    = worldX;
							closestY    = worldY;
							found       = true;
						}
					}
				}
			}
		}

		if (found)
		{
			printf("Found starting home marker at %d,%d\n", closestX, closestY);
			addHomeLocation(closestX, closestY);
		}
	}

	mFirstServerMessagesReceived |= 1;
}

if (mapPullMode)
{

	if (x == mapPullCurrentX - sizeX / 2 && y == mapPullCurrentY - sizeY / 2)
	{

		lastScreenViewCenter.x = mapPullCurrentX * CELL_D;
		lastScreenViewCenter.y = mapPullCurrentY * CELL_D;
		setViewCenterPosition(lastScreenViewCenter.x, lastScreenViewCenter.y);

		mapPullCurrentX += 10;

		if (mapPullCurrentX > mapPullEndX)
		{
			mapPullCurrentX = mapPullStartX;
			mapPullCurrentY += 5;

			if (mapPullCurrentY > mapPullEndY) { mapPullModeFinalImage = true; }
		}
		mapPullCurrentSaved = false;
		mapPullCurrentSent  = false;
	}
}
