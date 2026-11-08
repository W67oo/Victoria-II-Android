import java.util.zip.CRC32

plugins {
    id("com.android.application")
}

val aliceAssetsSource = rootProject.file("../src/engine/assets")
val aliceAssetsOut = layout.buildDirectory.dir("generated/aliceAssets")

val prepareAliceAssets by tasks.registering {
    inputs.dir(aliceAssetsSource)
    outputs.dir(aliceAssetsOut)
    doLast {
        val out = aliceAssetsOut.get().asFile
        out.deleteRecursively()
        val dest = File(out, "assets")
        aliceAssetsSource.copyRecursively(dest, overwrite = true)
        val luaEngineDir = rootProject.file("../build-android/generated/lua_engine")
        if (luaEngineDir.exists()) {
            luaEngineDir.copyRecursively(File(dest, "lua/engine"), overwrite = true)
        }
        val manifest = dest.walkTopDown()
            .filter { it.isFile }
            .map {
                val crc = CRC32()
                crc.update(it.readBytes())
                it.relativeTo(out).invariantSeparatorsPath + "\t" + it.length() + "\t" + crc.value.toString(16)
            }
            .sorted()
            .joinToString("\n")
        File(out, "alice_assets_manifest.txt").writeText(manifest + "\n")
    }
}

android {
    namespace = "com.vic2mobile.app"
    compileSdk = 34
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "com.vic2mobile.app"
        minSdk = 30
        targetSdk = 34
        versionCode = 1
        versionName = "0.2.0-beta"

        ndk {
            abiFilters += listOf("arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++20")
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DBUILD_HOST_GENERATORS_ONLY=OFF",
                    "-DVIC2_DISABLE_MULTIPLAYER=ON",
                    "-DHOST_TOOLS_BIN_DIR=${rootProject.projectDir.parentFile.resolve("build-host").absolutePath.replace('\\', '/')}"
                )
            }
        }
    }

    sourceSets {
        getByName("main") {
            assets.srcDir(aliceAssetsOut)
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.30.5"
            buildStagingDirectory = file("../../build-cxx")
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }

    androidResources {
        ignoreAssetsPattern = "!.svn:!.git:!.ds_store:!*.scc:!CVS:!thumbs.db:!picasa.ini:!*~"
        noCompress += listOf("txt", "ttf", "otf", "aui", "tui", "asvg", "dds", "png", "wav", "mp3")
    }

    buildTypes {
        debug {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
        }
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

tasks.named("preBuild") {
    dependsOn(prepareAliceAssets)
}
