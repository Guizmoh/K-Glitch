package com.somnia.app.data

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase

@Database(entities = [SleepSession::class, SoundEventEntity::class, MinuteStat::class], version = 2, exportSchema = false)
abstract class SomniaDatabase : RoomDatabase() {
    abstract fun dao(): SleepDao

    companion object {
        /** v2 : transcription, corrections, calibration (les nuits existantes sont conservées). */
        val MIGRATION_1_2 = object : androidx.room.migration.Migration(1, 2) {
            override fun migrate(db: androidx.sqlite.db.SupportSQLiteDatabase) {
                db.execSQL("ALTER TABLE events ADD COLUMN transcript TEXT")
                db.execSQL("ALTER TABLE events ADD COLUMN userCorrected INTEGER NOT NULL DEFAULT 0")
                db.execSQL("ALTER TABLE sessions ADD COLUMN movementMedian REAL")
                db.execSQL("ALTER TABLE sessions ADD COLUMN calibrationNights INTEGER NOT NULL DEFAULT 0")
            }
        }

        @Volatile private var instance: SomniaDatabase? = null
        fun get(context: Context): SomniaDatabase = instance ?: synchronized(this) {
            instance ?: Room.databaseBuilder(context.applicationContext, SomniaDatabase::class.java, "somnia.db")
                .addMigrations(MIGRATION_1_2)
                .build().also { instance = it }
        }
    }
}
