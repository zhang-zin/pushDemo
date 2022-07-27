package com.zj26.pushdemo.push

import android.hardware.Camera
import android.util.Log
import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import com.zj26.pushdemo.LivePusher

class VideoChannel(
    private val livePusher: LivePusher,
    activity: ComponentActivity,
    width: Int,
    height: Int,
    private val bitrate: Int,
    private val fps: Int,
    cameraID: Int
) : Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {

    private var isLiving: Boolean = false
    private val cameraHelper: CameraHelper

    init {
        cameraHelper = CameraHelper(activity, cameraID, width, height)
        cameraHelper.setPreviewCallback(this)
        cameraHelper.setOnChangedSizeListener(this)
    }

    override fun onPreviewFrame(data: ByteArray, camera: Camera) {
        if (isLiving){
            livePusher.native_pushVideo(data)
        }
    }

    override fun onChanged(w: Int, h: Int) {
        livePusher.native_setVideoEncInfo(w, h, fps, bitrate)
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        cameraHelper.switchCamera()
    }

    fun startLive() {
        isLiving = true
    }

    fun stopLive(){
        isLiving = false
    }

}