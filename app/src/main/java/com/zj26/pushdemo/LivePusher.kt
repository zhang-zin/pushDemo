package com.zj26.pushdemo

import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import com.zj26.pushdemo.push.AudioChannel
import com.zj26.pushdemo.push.VideoChannel

class LivePusher(
    activity: ComponentActivity,
    width: Int,
    height: Int,
    bitrate: Int,
    fps: Int,
    cameraID: Int
) {
    companion object {
        init {
            System.loadLibrary("pushdemo")
        }
    }

    private val audioChannel: AudioChannel
    private val videoChannel: VideoChannel

    init {
        native_init()
        videoChannel = VideoChannel(this, activity, width, height, bitrate, fps, cameraID)
        audioChannel = AudioChannel(this, activity)
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        videoChannel.switchCamera()
    }

    fun startLive() {
        native_start("rtmp://124.70.105.64/myapp")
        videoChannel.startLive()
        audioChannel.startLive()
    }

    fun stopLive() {
        release()
        audioChannel.release()
        videoChannel.stopLive()
    }

    private external fun native_init()
    private external fun native_start(url: String)
    private external fun release()

    external fun native_setVideoEncInfo(width: Int, height: Int, fps: Int, bitrate: Int)
    external fun native_pushVideo(data: ByteArray)
    external fun getInputSamples(): Int
    external fun native_setAudioEncInfo(sampleRateInHz: Int, channel: Int)
    external fun native_pushAudio(bytes: ByteArray)
}