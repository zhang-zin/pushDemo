package com.zj26.pushdemo.push

import android.app.Activity
import android.hardware.Camera
import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import com.zj26.pushdemo.LivePusher

class VideoChannel(
    livePusher: LivePusher,
    activity: ComponentActivity,
    width: Int,
    height: Int,
    bitrate: Int,
    fps: Int,
    cameraID: Int
) : Camera.PreviewCallback, CameraHelper.OnChangedSizeListener {

    private val cameraHelper: CameraHelper

    init {
        cameraHelper = CameraHelper(activity, cameraID, width, height)
        cameraHelper.setPreviewCallback(this)
        cameraHelper.setOnChangedSizeListener(this)
    }

    override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
    }

    override fun onChanged(w: Int, h: Int) {

    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        cameraHelper.setPreviewDisplay(surfaceHolder)
    }

    fun switchCamera() {
        cameraHelper.switchCamera()
    }


}