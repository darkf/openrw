#include <engine/GameData.hpp>
#include <engine/GameWorld.hpp>
#include <loaders/LoaderIPL.hpp>
#include <loaders/LoaderDFF.hpp>
#include <loaders/LoaderIDE.hpp>
#include <render/TextureAtlas.hpp>
#include <loaders/LoaderCOL.hpp>
#include <data/ObjectData.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <dirent.h>
#include <sys/types.h>

/**
 * Finds the 'real' case for a path, to get around the fact that Rockstar's data is usually the wrong case.
 * @param base The base of the path to start looking from.
 * @param path the lowercase path.
 */
std::string findPathRealCase(const std::string& base, const std::string& path) 
{
#ifdef __unix__
	size_t endslash = path.find("/");
	bool isDirectory = true;
	if(endslash == path.npos) {
		isDirectory = false;
	}
	std::string orgFileName = isDirectory ? path.substr(0, endslash) : path;
	std::string realName;
	
	// Open the current "base" path (i.e. the real path)
	DIR* dp = opendir(base.c_str());
	dirent* ep;
		
	if( dp != NULL) {
		while( (ep = readdir(dp)) ) {
			realName = ep->d_name;
			std::string lowerRealName = realName;
			std::transform(lowerRealName.begin(), lowerRealName.end(), lowerRealName.begin(), ::tolower);
			if( lowerRealName == orgFileName) {
				closedir(dp);
				if( isDirectory) {
					return findPathRealCase(base + "/" + realName, path.substr(endslash+1));
				}
				else {
					return base + "/" + realName;
				}
			}
		}
	}
	
	return "";
#else 
	// Is anything other than Windows likely to fall here?
	return path;
#endif
}

// Yet another hack function to fix these paths
std::string fixPath(std::string path) {
	for( size_t t = 0; t < path.size(); ++t) {
		path[t] = tolower(path[t]);
		if(path[t] == '\\') {
			path[t] = '/';
		}
	}
	return path;
}


GameData::GameData(const std::string& path)
: datpath(path), engine(nullptr)
{

}

void GameData::load()
{
	parseDAT(datpath+"/data/default.dat");
	parseDAT(datpath+"/data/gta3.dat");
	
	_knownFiles.insert({"wheels.DFF", {false, datpath+"/models/Generic/wheels.DFF"}});
	_knownFiles.insert({"loplyguy.dff", {false, datpath+"/models/Generic/loplyguy.dff"}});
	_knownFiles.insert({"particle.txd", {false, datpath+"/models/particle.txd"}});
	loadDFF("wheels.DFF");
	loadTXD("particle.txd");
	
	loadCarcols(datpath+"/data/carcols.dat");
	loadWeather(datpath+"/data/timecyc.dat");
	loadHandling(datpath+"/data/handling.cfg");
	loadWaterpro(datpath+"/data/waterpro.dat");
	loadWater(datpath+"/data/water.dat");

	loadIFP(datpath+"/anim/ped.ifp");
}

void GameData::parseDAT(const std::string& path)
{
	std::ifstream datfile(path.c_str());
	
	if(!datfile.is_open()) 
	{
		std::cerr << "Failed to open gta.dat" << std::endl;
	}
	else
	{
		for(std::string line, cmd; std::getline(datfile, line);)
		{
			if(line.size() == 0 || line[0] == '#') continue;
			line.erase(line.size()-1);
			
			size_t space = line.find_first_of(' ');
			if(space != line.npos)
			{
				cmd = line.substr(0, space);
				if(cmd == "IDE")
				{
					loadIDE(line.substr(space+1));
				}
				else if(cmd == "SPLASH")
				{
					splash = line.substr(space+1);
				}
				else if(cmd == "COLFILE")
				{
					int zone  = atoi(line.substr(space+1,1).c_str());
					std::string file = line.substr(space+3);
					loadCOL(zone, file);
				}
				else if(cmd == "IPL")
				{
					std::string fixedpath = fixPath(line.substr(space+1));
					fixedpath = findPathRealCase(datpath, fixedpath);
					loadIPL(fixedpath);
				}
				else if(cmd == "TEXDICTION") 
				{
					std::string texpath = line.substr(space+1);
					for( size_t t = 0; t < texpath.size(); ++t) {
						texpath[t] = tolower(texpath[t]);
						if(texpath[t] == '\\') {
							texpath[t] = '/';
						}
					}
					texpath = findPathRealCase(datpath, texpath);
					std::string texname = texpath.substr(texpath.find_last_of("/")+1);
					_knownFiles.insert({ texname, { false, texpath }});
					loadTXD(texname);
				}
			}
		}
	}
}

void GameData::loadIDE(const std::string& name)
{
	std::string lowername = name;
	for(size_t t = 0; t < lowername.size(); ++t)
	{
		lowername[t] = tolower(lowername[t]);
		if(lowername[t] == '\\') {
			lowername[t] = '/';
		}
	}
	
	ideLocations.insert({lowername, datpath+"/"+lowername});
}

void GameData::loadCOL(const size_t zone, const std::string& name)
{
	LoaderCOL col;
	
	std::string realPath = fixPath(name);
	realPath = findPathRealCase(datpath, realPath);
	
	if(col.load(realPath)) {
		for( size_t i = 0; i < col.instances.size(); ++i ) {
			collisions[col.instances[i]->name] = std::move(col.instances[i]);
		}
	}
}

void GameData::loadIMG(const std::string& name)
{
	LoaderIMG imgLoader; 
	std::string archivePath = datpath + name;

	if (imgLoader.load(archivePath)) {
		for (size_t i = 0; i < imgLoader.getAssetCount(); i++) {
			auto &asset = imgLoader.getAssetInfoByIndex(i);

			std::string filename = asset.name;
			
			if(asset.size == 0)
			{
				std::cerr << "Asset " << filename << " has no size, ignoring." << std::endl;
			}
			else
            {
				// Enter the asset twice.. 
				_knownFiles.insert({ filename, { true, archivePath }});
				for(size_t t = 0; t < filename.size(); ++t)
				{
					filename[t] = tolower(filename[t]);
				}
				_knownFiles.insert({ filename, { true, archivePath }});
			}
		}
		archives.insert({archivePath, imgLoader});
	}
}

void GameData::loadIPL(const std::string& name)
{
	iplLocations.insert({name, name});
}

enum ColSection {
	Unknown,
	COL,
	CAR,
	CAR3, ///> Used in GTASA, contains extra specular color
	CAR4  ///> Used in GTASA, contains quadruple colors
};

void GameData::loadCarcols(const std::string& path)
{
	std::ifstream fstream(path.c_str());
	
	std::string line;
	ColSection currentSection = Unknown;
	while( std::getline(fstream, line)) {
		if( line.substr(0, 1) == "#") { // Comment 
			continue; 
		}
		else if( currentSection == Unknown) {
			if( line.substr(0, 3) == "col") {
				currentSection = COL;
			}
			else if( line.substr(0, 3) == "car") {
				currentSection = CAR;
			}
		}
		else if( line.substr(0, 3) == "end") {
			currentSection = Unknown;
		}
		else {
			if( currentSection == COL) {
				std::string r, g, b;
				std::stringstream ss(line);
				
				if( std::getline(ss, r, ',') && std::getline(ss, g, ',') && std::getline(ss, b)) {
					vehicleColours.push_back(glm::vec3(
						atoi(r.c_str())/255.f,
						atoi(g.c_str())/255.f,
						atoi(b.c_str())/255.f
					));
				}
			}
			else if( currentSection == CAR) {
				std::string vehicle, p, s;
				std::stringstream ss(line);
				
				std::getline(ss, vehicle, ',');
				std::vector<std::pair<size_t, size_t>> colours;
				
				while( std::getline(ss, p, ',') && std::getline(ss, s, ',') ) {
					colours.push_back({ atoi(p.c_str()), atoi(s.c_str()) });
				}
				
				vehiclePalettes.insert({vehicle, colours});
			}
		}
	}
}

void GameData::loadWeather(const std::string &path)
{
	weatherLoader.load(path);
}

void GameData::loadHandling(const std::string& path)
{
	std::ifstream hndFile(path.c_str());

	if(! hndFile.is_open()) {
		std::cerr << "Error loadind handling data " << path << std::endl;
		return;
	}

	std::string lineBuff;

	while(std::getline(hndFile, lineBuff)) {
		if(lineBuff.at(0) == ';') continue;
		std::stringstream ss(lineBuff);

		VehicleHandlingInfo info;
		ss >> info.ID;
		ss >> info.mass;
		ss >> info.dimensions.x;
		ss >> info.dimensions.y;
		ss >> info.dimensions.z;
		ss >> info.centerOfMass.x;
		ss >> info.centerOfMass.y;
		ss >> info.centerOfMass.z;
		ss >> info.percentSubmerged;
		ss >> info.tractionMulti;
		ss >> info.tractionLoss;
		ss >> info.tractionBias;
		ss >> info.numGears;
		ss >> info.maxVelocity;
		ss >> info.acceleration;
		char dt, et;
		ss >> dt; ss >> et;
		info.driveType = (VehicleHandlingInfo::DriveType)dt;
		info.engineType = (VehicleHandlingInfo::EngineType)et;
		ss >> info.brakeDeceleration;
		ss >> info.brakeBias;
		ss >> info.ABS;
		ss >> info.steeringLock;
		ss >> info.suspensionForce;
		ss >> info.suspensionDamping;
		ss >> info.seatOffset;
		ss >> info.damageMulti;
		ss >> info.value;
		ss >> info.suspensionUpperLimit;
		ss >> info.suspensionLowerLimit;
		ss >> info.suspensionBias;
		ss >> std::hex >> info.flags;

		auto mit = vehicleInfo.find(info.ID);
		if(mit == vehicleInfo.end()) {
			vehicleInfo.insert({info.ID,
								VehicleInfoHandle(new VehicleInfo{info})});
		}
		else {
			mit->second->handling = info;
		}
	}
}

void GameData::loadWaterpro(const std::string& path)
{
	std::ifstream ifstr(path.c_str());
	
	if(ifstr.is_open()) {
		uint32_t numlevels; 
		ifstr.read(reinterpret_cast<char*>(&numlevels), sizeof(uint32_t));
		ifstr.read(reinterpret_cast<char*>(&waterHeights), sizeof(float)*48);
		
		ifstr.seekg(0x03C4);
		ifstr.read(reinterpret_cast<char*>(&visibleWater), sizeof(float)*64*64);
	}
}

void GameData::loadWater(const std::string& path)
{
	std::ifstream ifstr(path.c_str());
	
	std::string line;
	while( std::getline(ifstr, line)) {
		if( line[0] == ';') {
			continue;
		}
		
		std::stringstream ss(line);
		
		std::string a, b, c, d, e;
		float fa, fb, fc, fd, fe;
		
		if( std::getline(ss, a, ',') &&  std::getline(ss, b, ',') &&  std::getline(ss, c, ',') &&  std::getline(ss, d, ',') &&  std::getline(ss, e, ',')) {
			fa = atof(a.c_str());
			fb = atof(b.c_str());
			fc = atof(c.c_str());
			fd = atof(d.c_str());
			fe = atof(e.c_str());
			waterRects.push_back({
				fa,
				fb,
				fc,
				fd,
				fe
			});
		}
	}
}

void GameData::loadTXD(const std::string& name, bool async)
{
	if( loadedFiles.find(name) != loadedFiles.end() ) {
		return;
	}

	loadedFiles[name] = true;

	auto j = new LoadTextureArchiveJob(this->engine->_work, this, name);

	if( async ) {
		this->engine->_work->queueJob( j );
	}
	else {
		j->work();
		j->complete();
		delete j;
	}
}

void GameData::loadDFF(const std::string& name, bool async)
{
	auto realname = name.substr(0, name.size() - 4);
	if( models.find(realname) != models.end() ) {
		return;
	}

	// Before starting the job make sure the file isn't loaded again.
	loadedFiles.insert({name, true});

	models[realname] = new ModelHandle(realname);

	auto j = new LoadModelJob(this->engine->_work, this, name,
							  [&, realname]( Model* model ) {
								  models[realname]->model = model;
							  }
						  );


	if( async ) {
		this->engine->_work->queueJob( j );
	}
	else {
		j->work();
		j->complete();
		delete j;
	}

}

void GameData::loadIFP(const std::string &name)
{
    std::ifstream dfile(name.c_str());

    if(dfile.is_open())
    {
        dfile.seekg(0, std::ios_base::end);
        size_t length = dfile.tellg();
        dfile.seekg(0);
        char *file = new char[length];
        dfile.read(file, length);

        LoaderIFP loader;
        if( loader.loadFromMemory(file) ) {
            animations.insert(loader.animations.begin(), loader.animations.end());
        }

        delete[] file;
    }
}

void GameData::loadDynamicObjects(const std::string& name)
{
	std::ifstream dfile(name.c_str());
	
	if(dfile.is_open()) {
		std::string lineBuff;

		while(std::getline(dfile, lineBuff)) {
			if(lineBuff.at(0) == ';') continue;
			std::stringstream ss(lineBuff);
			
			std::shared_ptr<DynamicObjectData> dyndata(new DynamicObjectData);
			
			ss >> dyndata->modelName;
			ss >> dyndata->mass;
			ss >> dyndata->turnMass;
			ss >> dyndata->airRes;
			ss >> dyndata->elacticity;
			ss >> dyndata->bouancy;
			ss >> dyndata->uprootForce;
			ss >> dyndata->collDamageMulti;
			ss >> dyndata->collDamageFlags;
			ss >> dyndata->collResponseFlags;
			ss >> dyndata->cameraAvoid;
			
			dynamicObjectData.insert({dyndata->modelName, dyndata});
		}

	}
	else {
		engine->logError("Failed to load dynamic object file: " + name);
	}
}

char* GameData::openFile(const std::string& name)
{
	auto i = _knownFiles.find(name);
	if(i != _knownFiles.end())
	{
		if(i->second.archived)
		{
			// Find the archive
			auto ai = archives.find(i->second.path);
			if(ai != archives.end())
			{
				return ai->second.loadToMemory(name);
			}
			else 
			{
				std::cerr << "Archive not found " << i->second.path << std::endl;
			}
		}
		else
		{
			std::ifstream dfile(i->second.path);
			if ( ! dfile.is_open()) {
				std::cerr << "Error opening file " << i->second.path << std::endl;
				return nullptr;
			}

			dfile.seekg(0, std::ios_base::end);
			size_t length = dfile.tellg();
			dfile.seekg(0);
			char *data = new char[length];
			dfile.read(data, length);
			
			return data;
		}
	}
	else 
	{
		std::stringstream err;
		err << "Unable to locate file " << name;
		engine->logError(err.str());
		std::cerr << err.str() << std::endl;
	}
	
	return nullptr;
}

char* GameData::loadFile(const std::string &name)
{
	auto it = loadedFiles.find(name);
	 if( it != loadedFiles.end() ) {
		std::cerr << "File " << name << " already loaded?" << std::endl;
	}

	loadedFiles[name] = true;

	return openFile(name);
}

TextureAtlas* GameData::getAtlas(size_t i)
{
	if( i < atlases.size() ) {
		return atlases[i];
	}
	if( i == atlases.size() && (i == 0 || atlases[i-1]->getTextureCount() > 0) ) {
		GLint max_texture_size;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
		atlases.push_back(new TextureAtlas(max_texture_size/2, max_texture_size/2));
		return atlases[i];
	}
	return nullptr;
}
