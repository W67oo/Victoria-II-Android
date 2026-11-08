plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val generatedAliceAssetsDir = layout.buildDirectory.dir("generated/alice_assets")

val syncAliceBuiltinAssets by tasks.registering(Sync::class) {
    from("../../src/engine/assets") {
        into("alice_builtin/assets")
    }
    from("../../build-android/generated/lua_engine") {
        into("alice_builtin/assets/lua/engine")
    }
    into(generatedAliceAssetsDir)
}

android {
    namespace = "com.vic2mobile.app"
    compileSdk = 34
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "com.vic2mobile.app"
        minSdk = 29
        targetSdk = 34
        versionCode = 1
        versionName = "0.1.0-alpha"

        ndk {
            // 64-bit ARMv8 only: required for >4GB virtual address space & ARM NEON SIMD
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
            java.srcDir("../../upstream/SDL2/android-project/app/src/main/java")
            assets.srcDir(generatedAliceAssetsDir)
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

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

tasks.named("preBuild") {
    dependsOn(syncAliceBuiltinAssets)
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
}
