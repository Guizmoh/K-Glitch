package com.somnia.app.data

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase

@Database(entities = [SleepSession::class, SoundEventEntity::class, MinuteStat::class], version = 1, exportSchema = false)
abstract class SomniaDatabase : RoomDatabase() {
    abstract fun dao(): SleepDao

    companion object {
        @Volatile private var instance: SomniaDatabase? = null
        fun get(context: Context): SomniaDatabase = instance ?: synchronized(this) {
            instance ?: Room.databaseBuilder(context.applicationContext, SomniaDatabase::class.java, "somnia.db")
                .build().also { instance = it }
        }
    }
}
