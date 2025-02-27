cmake_minimum_required(VERSION 3.13)

# This tells cmake we have goodies in the /cmake folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PamplejuceVersion)

# Modern concise way to add dependencies to your project
include(CPM)

# Configures universal binaries and decides which version of macOS to support
include(PamplejuceMacOS)

# Couple tweaks that IMO should be JUCE defaults
include(JUCEDefaults)

# Remove BUILD_SHARED_LIBS for libdatachannel
set(BUILD_SHARED_LIBS OFF)
# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
# Note: This cannot have spaces (it may be 2024, but you can't have it all!)
# Worry not, JUCE's PRODUCT_NAME can have spaces (and is what DAWs display)
set(PROJECT_NAME "MeloVST")
set(PRODUCT_NAME "Melo VST")
set(USE_OPEN_SSL ON)
set(USE_TLS ON)
set(COMPANY_NAME "Melo Company")
set(BUNDLE_ID "com.melo.MeloVST")
set(FORMATS Standalone AU VST3 AUv3)
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add CLAP format
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

# Add any other modules you want modules here, before the juce_add_plugin call
file(GLOB_RECURSE r8brainSourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/r8brain/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/modules/r8brain/*.h")

juce_add_binary_data(r8brain SOURCES ${r8brainSourceFiles})
#add_library(modules/r8brain STATIC)
#target_sources(modules/r8brain PRIVATE "${r8brain_SOURCE_DIR}/r8bbase.cpp")

# This adds the melatonin inspector module
add_subdirectory(modules/ssl)
add_subdirectory(modules/libdatachannel)
add_subdirectory(modules/ixwebsocket)
add_subdirectory(modules/opus)
add_subdirectory(modules/melatonin_inspector)

find_package(OpenSSL REQUIRED)
target_link_libraries(ixwebsocket PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon.png"
        COMPANY_NAME "${COMPANY_NAME}"
        BUNDLE_ID "${BUNDLE_ID}"
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Melo
        PLUGIN_CODE M001
        FORMATS "${FORMATS}"
        PRODUCT_NAME "${PRODUCT_NAME}"
        COMPANY_WEBSITE "https://studio-melo.com"
        COMPANY_EMAIL "contact@studio-melo.com"
        DESCRIPTION "Melo Plugin to send the music to the artist"
        #        HARDENED_RUNTIME_ENABLED TRUE
)

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

clap_juce_extensions_plugin(TARGET "${PROJECT_NAME}"
        CLAP_ID "${BUNDLE_ID}"
        CLAP_FEATURES audio-effect)

include(SharedCodeDefaults)


# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# Adds a BinaryData target for embedding assets into the binary
include(Assets)

# MacOS only: Cleans up folder and target organization on Xcode.
include(XcodePrettify)

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
        INTERFACE

        # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
        JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_ADD_R8BRAIN=1
        # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
        # JUCE_DISPLAY_SPLASH_SCREEN=0

        # lets the app known if we're Debug or Release
        CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
        VERSION="${CURRENT_VERSION}"

        # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
        PRODUCT_NAME_WITHOUT_VERSION="MeloVST"
)

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
        INTERFACE
        opus
        datachannel
        datachannel-static
        #    r8brain
        ixwebsocket
        Assets
        melatonin_inspector
        juce_audio_utils
        juce_audio_processors
        juce_dsp
        juce_gui_basics
        juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

MESSAGE(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # Créer le dossier Resources dans le bundle
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "MeloVST_artefacts/Debug/VST3/MeloVST.vst3/Contents/MacOS"
        # Copier le fichier certificate dans Resources
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/ca-certificates.crt"
        "MeloVST_artefacts/Debug/VST3/MeloVST.vst3/Contents/MacOS/ca-certificates.crt"
        COMMENT "Copie du certificat dans le bundle VST3 (Contents/Resources)"
)

# IPP support, comment out to disable
include(PamplejuceIPP)
# Everything related to the tests target
include(Tests)
# A separate target for Benchmarks (keeps the Tests target fast)
include(Benchmarks)
# Output some config for CI (like our PRODUCT_NAME)
include(GitHubENV)


get_target_property(IXWEB_INCLUDE ixwebsocket INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "IXWEB_INCLUDE: ${IXWEB_INCLUDE}")
if (IXWEB_INCLUDE)
    # Sépare la chaîne en une liste d'arguments
    separate_arguments(IXWEB_INCLUDE_LIST UNIX_COMMAND "${IXWEB_INCLUDE}")
    set(new_includes "")
    foreach(item IN LISTS IXWEB_INCLUDE_LIST)
        message(STATUS "item: ${item}")
        # Vérifie si item contient le chemin à supprimer
        string(FIND "${item}" "modules/ixwebsocket" pos)
        if (pos EQUAL -1)
            list(APPEND new_includes "${item}")
        else()
            message(STATUS "Removing: ${item}")
        endif()
    endforeach()
    message(STATUS "new_includes: ${new_includes}")
    set_target_properties(ixwebsocket PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${new_includes}")
endif()