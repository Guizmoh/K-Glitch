import java.net.URI

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
    id("com.google.devtools.ksp")
    // Captures d'écran des interfaces Compose sur la JVM (./gradlew recordPaparazziDebug)
    id("app.cash.paparazzi")
}

// Modèle YAMNet (Google, Apache 2.0) : 521 classes de sons dont « Speech »,
// « Snoring », « Laughter », « Crying, sobbing », « Screaming »...
// Il est téléchargé à la compilation pour ne pas versionner un binaire de 4 Mo.
val yamnetUrl = "https://storage.googleapis.com/mediapipe-models/audio_classifier/yamnet/float32/1/yamnet.tflite"
val generatedAssets = layout.buildDirectory.dir("generated/yamnet-assets")

val downloadYamnet by tasks.registering {
    val out = generatedAssets.map { it.file("yamnet.tflite") }
    outputs.file(out)
    doLast {
        val f = out.get().asFile
        if (!f.exists() || f.length() < 1_000_000) {
            f.parentFile.mkdirs()
            logger.lifecycle("Téléchargement du modèle YAMNet…")
            URI(yamnetUrl).toURL().openStream().use { input ->
                f.outputStream().use { input.copyTo(it) }
            }
        }
    }
}

android {
    namespace = "com.somnia.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.policedudodo.app"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
    }

    sourceSets["main"].assets.srcDir(generatedAssets)

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    buildFeatures { compose = true }
    androidResources { noCompress += "tflite" }

    // Nom du fichier APK : police-du-dodo.apk (release : police-du-dodo-release.apk)
    applicationVariants.all {
        val variant = this
        outputs.all {
            (this as com.android.build.gradle.internal.api.BaseVariantOutputImpl).outputFileName =
                if (variant.buildType.name == "debug") "police-du-dodo.apk" else "police-du-dodo-${variant.buildType.name}.apk"
        }
    }
}

tasks.named("preBuild") { dependsOn(downloadYamnet) }

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2024.12.01")
    implementation(composeBom)
    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.activity:activity-compose:1.9.3")
    implementation("androidx.lifecycle:lifecycle-runtime-compose:2.8.7")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.7")
    implementation("androidx.lifecycle:lifecycle-service:2.8.7")
    implementation("androidx.navigation:navigation-compose:2.8.5")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.compose.ui:ui-tooling-preview")
    debugImplementation("androidx.compose.ui:ui-tooling")

    implementation("androidx.room:room-runtime:2.6.1")
    implementation("androidx.room:room-ktx:2.6.1")
    ksp("androidx.room:room-compiler:2.6.1")

    implementation("com.google.mediapipe:tasks-audio:0.10.20")

    testImplementation("junit:junit:4.13.2")
}
