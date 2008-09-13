//
// C++ Interface: CompiledPresetFactory
//
// Description: 
//
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef __COMPILED_PRESET_FACTORY_HPP
#define __COMPILED_PRESET_FACTORY_HPP

#include <memory>
#include "PresetFactory.hpp"

class PresetLibrary;

class CompiledPresetFactory : public PresetFactory {

public:

 CompiledPresetFactory();

 virtual ~CompiledPresetFactory();

 virtual std::auto_ptr<Preset> allocate(const std::string & url, const std::string & name = std::string(), 
	const std::string & author = std::string());

 std::string extension() const { return "so"; }

private:
	PresetLibrary * loadLibrary(const std::string & url);
	typedef std::map<std::string, PresetLibrary*> PresetLibraryMap;
	PresetLibraryMap _libraries;
	
};

#endif
