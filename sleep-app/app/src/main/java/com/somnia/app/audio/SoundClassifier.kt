package com.somnia.app.audio

/** Classe une fenêtre de 0,975 s (15 600 échantillons à 16 kHz, valeurs -1..1). */
interface SoundClassifier : AutoCloseable {
    val name: String
    fun classify(window: FloatArray): SoundScores
    override fun close() {}

    companion object {
        const val WINDOW = 15_600
    }
}
