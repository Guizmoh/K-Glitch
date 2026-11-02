package com.somnia.app.data

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import kotlinx.coroutines.flow.Flow

@Dao
interface SleepDao {
    @Insert suspend fun insertSession(s: SleepSession): Long
    @Update suspend fun updateSession(s: SleepSession)
    @Delete suspend fun deleteSession(s: SleepSession)

    @Query("SELECT * FROM sessions WHERE id = :id") suspend fun session(id: Long): SleepSession?
    @Query("SELECT * FROM sessions WHERE id = :id") fun sessionFlow(id: Long): Flow<SleepSession?>
    @Query("SELECT * FROM sessions ORDER BY startTime DESC") fun sessions(): Flow<List<SleepSession>>
    @Query("SELECT * FROM sessions WHERE endTime IS NOT NULL ORDER BY startTime DESC")
    suspend fun finishedSessions(): List<SleepSession>
    @Query("SELECT * FROM sessions WHERE endTime IS NULL ORDER BY startTime DESC LIMIT 1")
    suspend fun openSession(): SleepSession?

    @Insert suspend fun insertEvent(e: SoundEventEntity): Long
    @Update suspend fun updateEvent(e: SoundEventEntity)
    @Delete suspend fun deleteEvent(e: SoundEventEntity)
    @Query("SELECT * FROM events WHERE sessionId = :sid ORDER BY timestamp")
    fun events(sid: Long): Flow<List<SoundEventEntity>>
    @Query("SELECT * FROM events WHERE sessionId = :sid ORDER BY timestamp")
    suspend fun eventsList(sid: Long): List<SoundEventEntity>
    @Query("SELECT * FROM events")
    suspend fun allEvents(): List<SoundEventEntity>
    @Query("SELECT * FROM events WHERE timestamp < :before AND favorite = 0 AND audioPath IS NOT NULL")
    suspend fun oldRecordings(before: Long): List<SoundEventEntity>

    @Insert(onConflict = OnConflictStrategy.REPLACE) suspend fun upsertMinute(m: MinuteStat)
    @Insert(onConflict = OnConflictStrategy.REPLACE) suspend fun upsertMinutes(m: List<MinuteStat>)
    @Query("SELECT * FROM minutes WHERE sessionId = :sid ORDER BY minute")
    suspend fun minutes(sid: Long): List<MinuteStat>
    @Query("SELECT * FROM minutes WHERE sessionId = :sid ORDER BY minute")
    fun minutesFlow(sid: Long): Flow<List<MinuteStat>>
}
