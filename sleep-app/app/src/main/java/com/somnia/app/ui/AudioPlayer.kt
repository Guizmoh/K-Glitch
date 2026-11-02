package com.somnia.app.ui

import android.media.MediaPlayer
import android.os.Handler
import android.os.Looper
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

/** Lecteur unique : lancer un son arrête le précédent. Expose la position pour la barre de lecture. */
object AudioPlayer {
    private var player: MediaPlayer? = null
    private val handler = Handler(Looper.getMainLooper())

    var playingPath by mutableStateOf<String?>(null)
        private set
    var isPlaying by mutableStateOf(false)
        private set
    var positionMs by mutableIntStateOf(0)
        private set
    var durationMs by mutableIntStateOf(0)
        private set

    private val tick = object : Runnable {
        override fun run() {
            player?.let { if (it.isPlaying) positionMs = it.currentPosition }
            if (isPlaying) handler.postDelayed(this, 40)
        }
    }

    /** Lecture / pause du fichier [path] (reprend là où il était). */
    fun toggle(path: String) {
        if (playingPath == path && player != null) {
            if (isPlaying) pause() else resume()
            return
        }
        play(path, 0)
    }

    /** Démarre [path] à [startMs] (ou déplace la lecture si c'est déjà ce fichier). */
    fun play(path: String, startMs: Int) {
        if (playingPath == path && player != null) {
            seekTo(startMs); resume(); return
        }
        stop()
        player = runCatching {
            MediaPlayer().apply {
                setDataSource(path)
                setOnCompletionListener { AudioPlayer.isPlaying = false; AudioPlayer.positionMs = AudioPlayer.durationMs; handler.removeCallbacks(tick) }
                prepare()
                seekTo(startMs)
                start()
            }
        }.getOrNull()
        playingPath = if (player != null) path else null
        durationMs = player?.duration ?: 0
        positionMs = startMs
        isPlaying = player != null
        if (isPlaying) handler.post(tick)
    }

    fun seekTo(ms: Int) {
        player?.seekTo(ms.coerceIn(0, durationMs))
        positionMs = ms.coerceIn(0, durationMs)
    }

    private fun pause() {
        player?.pause(); isPlaying = false; handler.removeCallbacks(tick)
    }

    private fun resume() {
        val p = player ?: return
        if (positionMs >= durationMs) p.seekTo(0)
        p.start(); isPlaying = true; handler.post(tick)
    }

    fun stop() {
        handler.removeCallbacks(tick)
        player?.runCatching { stop(); release() }
        player = null
        playingPath = null
        isPlaying = false
        positionMs = 0
    }
}
