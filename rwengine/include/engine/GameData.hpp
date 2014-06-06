#pragma once
#ifndef _GAMEDATA_HPP_
#define _GAMEDATA_HPP_

#include <engine/GTATypes.hpp>
#include <loaders/LoaderIMG.hpp>
#include <loaders/TextureLoader.hpp>
#include <loaders/LoaderDFF.hpp>
#include <loaders/LoaderIDE.hpp>
#include <loaders/LoaderIFP.hpp>
#include <loaders/WeatherLoader.hpp>
#include <objects/VehicleInfo.hpp>
#include <data/CollisionModel.hpp>

#include <memory>

struct DynamicObjectData;
class GameWorld;
class TextureAtlas;

/**
 * @brief The TextureInfo struct
 * Contains metadata about where a texture can be found.
 */
struct TextureInfo
{
	/// Texture Name
	GLuint texName;
	
	/// Transparent flag.
	bool transparent;

	TextureInfo(GLuint tex, bool t)
		: texName(tex), transparent(t) {}
	TextureInfo()
		: texName(0), transparent(false) {} 
};

/**
 * Handles loading and management of the Game's DAT
 */
class GameData
{
private:
	
	std::string datpath;
	std::string splash;
	
public:
	
	/**
	 * @struct GTAFile
	 *  Stores information about a file the engine might want to load
	 */
	struct GTAFile
	{
		bool archived; /// Is the file inside an IMG or on the filesystem?
		std::string path; /// Path to the file containing the file.
	};
	
	/**
	 * ctor
	 * @param path Path to the root of the game data.
	 */
	GameData(const std::string& path = "");
	
	GameWorld* engine;
	
	/**
	 * Returns the current platform
	 */
	std::string getPlatformString()
	{
		return "PC";
	}
	
	/**
	 * Loads the data contained in the given file
	 */
	void loadIDE(const std::string& name);
	
	/**
	 * Handles the parsing of a COL file.
	 */
	void loadCOL(const size_t zone, const std::string& name);
	
	/**
	 * Handles the loading of an IMG's data
	 */
	void loadIMG(const std::string& name);
	
	void loadIPL(const std::string& name);
	
	void loadCarcols(const std::string& path);

	void loadWeather(const std::string& path);

	void loadHandling(const std::string& path);
	
	/**
	 * Loads water level data
	 */
	void loadWaterpro(const std::string& path);
	void loadWater(const std::string& path);
	
	void load();
	
	/**
	 * Loads a GTA3.dat file with the name path
	 */
	void parseDAT(const std::string& path);
	
	/**
	 * Attempts to load a TXD, or does nothing if it has already been loaded
	 */
	void loadTXD(const std::string& name, bool async = false);

	/**
	 * Attempts to load a DFF or does nothing if is already loaded
	 */
	void loadDFF(const std::string& name, bool async = false);

    /**
     * Loads an IFP file containing animations
     */
    void loadIFP(const std::string& name);
	
	/**
	 * Loads data from an object definition dat.
	 */
	void loadDynamicObjects(const std::string& name);

	/**
	 * Returns a pointer to the named file if it is available,
	 * the memory must be freed by the caller.
	 * @param name the filename in the archive
	 * @return pointer to the data, NULL if it is not available
	 */
	char* openFile(const std::string& name);

	/**
	 * @brief loadFile Marks a file as open, and opens it.
	 * @param name
	 * @return
	 */
	char* loadFile(const std::string& name);

	/**
	 * @brief getAtlas Returns atlas i, creating it if the situation calls for it.
	 *  "the situation" being the last atlas has more than 0 textures packed.
	 * @param i
	 * @return
	 */
	TextureAtlas* getAtlas(size_t i);
	
	/**
	 * Files that have been loaded previously
	 */
	std::map<std::string, bool> loadedFiles;
	
	/**
	 * Maps the paths in GTA3.dat to the real paths
	 */
	std::map<std::string, std::string> iplLocations;
	std::map<std::string, std::string> ideLocations;
	
	/**
	 * Maps file names to data about the file.
	 */
	std::map<std::string, GTAFile> _knownFiles;
	
	/**
	 * Map of loaded archives
	 */
	std::map<std::string, LoaderIMG> archives;
	
	/**
	 * The vehicle colour palettes
	 */
	std::vector<glm::vec3> vehicleColours;
	
	/**
	 * The vehicle colours for each vehicle type
	 */
	std::map<std::string, std::vector<std::pair<size_t,size_t>>> vehiclePalettes;

	/**
	 * Vehicle information
	 */
	std::map<std::string, VehicleInfoHandle> vehicleInfo;
	
	/**
	 * Texture Loader
	 */
	TextureLoader textureLoader;

	/**
	 * Weather Loader
	 */
	WeatherLoader weatherLoader;

	/**
	 * Loaded models
	 */
	std::map<std::string, ModelHandle*> models;

	/**
	 * Loaded Textures and their atlas entries.
	 */
	std::map<std::string, TextureInfo> textures;

	/**
	 * Texture atlases.
	 */
	std::vector<TextureAtlas*> atlases;

    /**
     * Loaded Animations
     */
    AnimationSet animations;

	/**
	 * CollisionModel data.
	 */
	std::map<std::string,  std::unique_ptr<CollisionModel>> collisions;
	
	/**
	 * DynamicObjectData 
	 */
	std::map<std::string, std::shared_ptr<DynamicObjectData>> dynamicObjectData;
	
	/**
	 * Water Rectangles
	 */
	std::vector<GTATypes::WaterRect> waterRects;
	
	/**
	 * Water heights
	 */
	float waterHeights[48];
	
	/**
	 * Visible water heights
	 */
	uint8_t visibleWater[64*64];
	
	/**
	 * The "real" water heights
	 */
	uint8_t realWater[128*128];
};

#endif
