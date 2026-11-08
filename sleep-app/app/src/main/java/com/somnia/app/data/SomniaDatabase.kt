package com.somnia.app.data

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase

@Database(entities = [SleepSession::class, SoundEventEntity::class, MinuteStat::class], version = 4, exportSchema = false)
abstract class SomniaDatabase : RoomDatabase() {
    abstract fun dao(): SleepDao

    companion object {
        /** v4 : inclinaison du téléphone par minute (positions de sommeil). */
        val MIGRATION_3_4 = object : androidx.room.migration.Migration(3, 4) {
            override fun migrate(db: androidx.sqlite.db.SupportSQLiteDatabase) {
                db.execSQL("ALTER TABLE minutes ADD COLUMN tilt REAL NOT NULL DEFAULT 0")
                db.execSQL("ALTER TABLE minutes ADD COLUMN pitch REAL NOT NULL DEFAULT 0")
                db.execSQL("ALTER TABLE minutes ADD COLUMN hasPosition INTEGER NOT NULL DEFAULT 0")
            }
        }

        /** v3 : nuits favorites et sons masqués (types désactivés). */
        val MIGRATION_2_3 = object : androidx.room.migration.Migration(2, 3) {
            override fun migrate(db: androidx.sqlite.db.SupportSQLiteDatabase) {
                db.execSQL("ALTER TABLE sessions ADD COLUMN favorite INTEGER NOT NULL DEFAULT 0")
                db.execSQL("ALTER TABLE events ADD COLUMN hidden INTEGER NOT NULL DEFAULT 0")
            }
        }

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
                .addMigrations(MIGRATION_1_2, MIGRATION_2_3, MIGRATION_3_4)
                .build().also { instance = it }
        }
    }
}
