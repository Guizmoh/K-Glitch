pluginManagement {
    repositories {
        google()
        // Miroir Google de Maven Central (plus rapide, évite les limitations de débit)
        maven("https://maven-central.storage-download.googleapis.com/maven2/")
        mavenCentral()
        gradlePluginPortal()
    }
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        // Miroir Google de Maven Central (plus rapide, évite les limitations de débit)
        maven("https://maven-central.storage-download.googleapis.com/maven2/")
        mavenCentral()
    }
}
rootProject.name = "PoliceDuDodo"
include(":app")
