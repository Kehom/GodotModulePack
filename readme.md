# Kehom's Godot Module Pack

This repository contains a collection of Godot modules, which are converted from the pure GDScript addon pack that can be found [here](https://github.com/Kehom/GodotAddonPack).

The modules in this pack were coded and tested using Godot v3.2.x.

The following addons have been converted to this module pack:

* General: Contains a few "general use" functionality.
* Network: Automates most of the networked multiplayer synchronization process, through snapshots. This module sort of "forces" the creation of an authoritative server system.

## Installation

Unfortunately to install a Godot module the entire engine has to be compiled from source code. Detailed information on how to compile the engine itself can be found [here](https://docs.godotengine.org/en/stable/development/compiling/index.html).

Once that is working, the desired module directories must be copied into the `modules` directory within the Godot directory. So, if you wanted to install the networking module, it would be placed in a subdirectory like `godot_source/modules/kehnetwork`.

After the files are within the proper subdirectories all that must be done is recompile the engine. The build system will automatically incorporate the modules into the final binary.

Advanced users often remove unnecessary modules from the builds. Note that the networking addon require the ENet module and/or WebSocket module. If both are missing the code will compile but the system will not properly work.

Some modules may have dependencies on other modules within this pack, like the `Network`, which require the `General` module.

Contrary to the GDScript addon pack, no module require activation within the Project Settings window. However, some modules may have usage slightly different from the addon. Such cases will be noted here in this readme.

As it happens with some of the addons in the GDScript version, some may add a few additional settings into the Project Settings window. In thta case a new category (`Keh Modules`) is added and, under it, an entry for the module, containing its settings.

## Tutorial

Generally speaking the information found on my web page [kehomsforge.com](http://kehomsforge.com/tutorials/multi/GodotAddonPack) is still relevant for the modules found in this pack (hey, those are "converted" from the scripted version). The very few cases where slightly differences are there in the usage will be noted here in this readme.

The main thing to keep in mind when reading the tutorial is that all the module classes are prefixed with a `keh`.


## The Modules

Bellow is a slightly more detailed list of the modules, including information regarding if the module has interdependency, has differences in how to use compared to the GDScript version and if it adds additional settings within the project settings.

### General

As mentioned this module is meant to contain some general use functionality.

#### kehEncDecBuffer

Interdependency | Extra Settings
-|-
none | no

Implements a class (`kehEncDecBuffer`) that wraps a `PoolByteArray` and provides means to add or extract data into the wrapped array. One of the features is that it allows "short integers" (8 or 16 bits) to be encoded/decoded. The main reason for this buffer to exist is to strip out variant headers (4 bytes per property) from encoded data, mostly for packing properties to be sent through networks.

However, this has been placed in the general module because maybe it can be useful for other things, like binary save files.

#### kehQuantize

Interdependency | Extra Settings
-|-
none | no

Provides means to quantize floating point numbers as well as compress rotation quaternions using the *smallest three* method. The entire functionality is provided through a singleton class, meaning that it's not necessary to explicitly create instances of the class `kehQuantize`. Although the returned quantized data are still using the full GDScript variant data, the resulting integers can be packed into others through bit masking. Also, this data can be directly used with the `kehEncDecBuffer` class, meaning that the two complement each other rather well.


### kehNetwork

Interdependency | Extra Settings
-|-
`kehEncDecBuffer`, `kehQuantize` | yes

This module was born in order to help create authoritative servers that send replication data to clients through snapshots and events. Most of the process is automated and the internal design is meant to be as "less intrusive as possible". The idea is to not completely change node hierarchy and have minimal impact on game logic.

#### Differences Between Addon and Module

* Initialization

When using the module version, the network system must be initialized with `kehNetwork.initialize()`.

* Snapshot Entities

Native classes exposed to scripting cannot have non default constructors. This means that classes deriving from `kehSnapEntityBase` cannot have the `_init()` function initializing the unique ID and class hash properties. To that end, a function has been created within the `kehNetwork` class meant to create instances of those snapshot entity objects with proper initialization. So, whenever an instance of that class is required it has to be done like this:

```
var some_entity: kehSnapEntityBase = kehNetwork.create_snap_entity(SnapShotEntityClass, [unique_id], [class_hash])
```
