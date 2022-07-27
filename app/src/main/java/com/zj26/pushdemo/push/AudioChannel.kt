package com.zj26.pushdemo.push

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import androidx.activity.ComponentActivity
import com.zj26.pushdemo.LivePusher
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

@SuppressLint("MissingPermission")
class AudioChannel(val livePusher: LivePusher, activity: ComponentActivity) {

    private val audioRecord: AudioRecord
    private var isLiving = false

    private var inputSamples = 0

    private val executor: ExecutorService by lazy {
        Executors.newSingleThreadExecutor()
    }

    init {
        val sampleRateInHz = 44100
        val minBufferSize = AudioRecord.getMinBufferSize(
            sampleRateInHz, AudioFormat.CHANNEL_IN_STEREO,
            AudioFormat.ENCODING_PCM_16BIT
        ) * 2
        livePusher.native_setAudioEncInfo(sampleRateInHz, 2)
        inputSamples = livePusher.getInputSamples() * 2
        val bufferSizeInBytes = if (inputSamples in 1 until minBufferSize) {
            inputSamples
        } else {
            minBufferSize
        }
        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.MIC,
            sampleRateInHz,
            AudioFormat.CHANNEL_IN_STEREO,
            AudioFormat.ENCODING_PCM_16BIT,
            bufferSizeInBytes
        )
    }

    fun startLive() {
        isLiving = true
        executor.submit(RecordTask())
    }

    fun release() {
        isLiving = false
        audioRecord.release()
    }

    inner class RecordTask : Runnable {
        override fun run() {
            audioRecord.startRecording()
            //    pcm  音频原始数据
            val bytes = ByteArray(inputSamples)
            while (isLiving) {
                audioRecord.read(bytes, 0, bytes.size)
                livePusher.native_pushAudio(bytes)
            }
        }
    }
}