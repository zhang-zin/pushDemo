package com.zj26.pushdemo

import android.app.Activity
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

    private val audioChannel: AudioChannel = AudioChannel(this)
    private val videoChannel: VideoChannel

    init {
        videoChannel = VideoChannel(this, activity, width, height, bitrate, fps, cameraID)
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        videoChannel.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        videoChannel.switchCamera()
    }
}