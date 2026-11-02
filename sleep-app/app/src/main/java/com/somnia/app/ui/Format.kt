package com.somnia.app.ui

import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

object Fmt {
    private val time = SimpleDateFormat("HH:mm", Locale.FRANCE)
    private val timeSec = SimpleDateFormat("HH:mm:ss", Locale.FRANCE)
    private val day = SimpleDateFormat("EEEE d MMMM", Locale.FRANCE)
    private val shortDay = SimpleDateFormat("EEE d", Locale.FRANCE)

    fun time(t: Long): String = time.format(Date(t))
    fun timeSec(t: Long): String = timeSec.format(Date(t))
    fun day(t: Long): String = day.format(Date(t)).replaceFirstChar { it.uppercase() }
    fun shortDay(t: Long): String = shortDay.format(Date(t))
    fun duration(min: Int): String = "${min / 60}h${"%02d".format(min % 60)}"
    fun seconds(ms: Long): String = if (ms < 60_000) "${ms / 1000} s" else "${ms / 60_000} min ${(ms / 1000) % 60} s"
}
