package com.somnia.app.ui

import android.media.MediaPlayer
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

/** Lecteur unique : lancer un son arrête le précédent. */
object AudioPlayer {
    private var player: MediaPlayer? = null
    var playingPath by mutableStateOf<String?>(null)
        private set

    fun toggle(path: String) {
        if (playingPath == path) { stop(); return }
        stop()
        player = runCatching {
            MediaPlayer().apply {
                setDataSource(path)
                setOnCompletionListener { stop() }
                prepare()
                start()
            }
        }.getOrNull()
        playingPath = if (player != null) path else null
    }

    fun stop() {
        player?.runCatching { stop(); release() }
        player = null
        playingPath = null
    }
}
