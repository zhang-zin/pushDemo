package com.zj26.pushdemo.push

import android.graphics.ImageFormat
import android.hardware.Camera
import android.hardware.Camera.CameraInfo
import android.hardware.Camera.PreviewCallback
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import kotlin.math.abs

class CameraHelper(
    private val activity: ComponentActivity,
    cameraID: Int,
    width: Int,
    height: Int
) : SurfaceHolder.Callback, Camera.PreviewCallback, LifecycleObserver {

    private var mCameraId = cameraID
    private var mCamera: Camera? = null
    private var mSurfaceHolder: SurfaceHolder? = null

    private var buffer: ByteArray? = null
    private var bytes: ByteArray? = null

    private var mWidth = width
    private var mHeight = height
    private var mRotation = 0

    private var onChangedSizeListener: OnChangedSizeListener? = null
    private var mPreviewCallback: PreviewCallback? = null

    init {
        activity.lifecycle.addObserver(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {

    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        stopPreview()
        startPreview()
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        stopPreview()
    }

    @Deprecated("Deprecated in Java")
    override fun onPreviewFrame(data: ByteArray, camera: Camera) {
        when (mRotation) {
            Surface.ROTATION_0 -> rotation90(data)
            Surface.ROTATION_90 -> {}
            Surface.ROTATION_270 -> {}
        }
        // data数据依然是倒的
        mPreviewCallback?.onPreviewFrame(bytes, camera)
        camera.addCallbackBuffer(buffer)
    }

    fun setPreviewCallback(previewCallback: PreviewCallback) {
        mPreviewCallback = previewCallback
    }

    fun setOnChangedSizeListener(onChangedSizeListener: OnChangedSizeListener) {
        this.onChangedSizeListener = onChangedSizeListener
    }

    fun setPreviewDisplay(surfaceHolder: SurfaceHolder) {
        mSurfaceHolder = surfaceHolder
        mSurfaceHolder?.addCallback(this)
    }

    fun switchCamera() {
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_FRONT
        } else {
            mCameraId = Camera.CameraInfo.CAMERA_FACING_BACK
        }
        stopPreview()
        startPreview()
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun release() {
        mSurfaceHolder?.removeCallback(this)
        stopPreview()
    }

    private fun startPreview() {
        kotlin.runCatching {
            // 获取camera对象
            mCamera = Camera.open(mCameraId).apply {
                // 配置camera的属性
                val parameters = parameters ?: return
                parameters.previewFormat = ImageFormat.NV21
                // 设置摄像头宽、高
                setPreviewSize(parameters)
                // 设置摄像头 图像传感器角度、方向
                setPreviewOrientation(this)
                setParameters(parameters)
                // 数据缓存区
                buffer = ByteArray(mWidth * mHeight * 3 / 2)
                bytes = ByteArray(buffer!!.size)
                addCallbackBuffer(buffer)
                setPreviewCallbackWithBuffer(this@CameraHelper)
                // 设置预览画面
                setPreviewDisplay(mSurfaceHolder)
                startPreview()
            }
        }.onFailure {
            Log.e("zj26", "it ${it.message}")
        }
    }

    private fun stopPreview() {
        mCamera?.let {
            it.setPreviewCallback(null)
            it.stopPreview()
            it.release()
            mCamera = null
        }
    }

    private fun setPreviewOrientation(camera: Camera) {
        val info = CameraInfo()
        Camera.getCameraInfo(mCameraId, info)
        mRotation = activity.windowManager.defaultDisplay.getRotation()
        var degrees = 0
        when (mRotation) {
            Surface.ROTATION_0 -> {
                degrees = 0
                onChangedSizeListener?.onChanged(mHeight, mWidth)
            }
            Surface.ROTATION_90 -> {
                degrees = 90
                onChangedSizeListener?.onChanged(mWidth, mHeight)
            }
            Surface.ROTATION_270 -> {
                degrees = 270
                onChangedSizeListener?.onChanged(mWidth, mHeight)
            }
        }
        var result: Int
        if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360
            result = (360 - result) % 360 // compensate the mirror
        } else { // back-facing
            result = (info.orientation - degrees + 360) % 360
        }
        //设置角度
        camera.setDisplayOrientation(result)
    }

    private fun setPreviewSize(parameters: Camera.Parameters) {
        val previewSizes = parameters.supportedPreviewSizes
        // 选择一个与设置的差距最小的支持分辨率
        var size = previewSizes[0]
        Log.d("zj26", "支持 " + size.width + "x" + size.height)
        var m = abs(size.height * size.width - mHeight * mWidth)
        previewSizes.removeAt(0)
        val iterator = previewSizes.iterator()
        while (iterator.hasNext()) {
            val next = iterator.next()
            val n = abs(size.height * size.width - mHeight * mWidth)
            if (n < m) {
                m = n
                size = next
            }
        }
        mWidth = size.width
        mHeight = size.height
        parameters.setPreviewSize(mWidth, mHeight)
        Log.d("zj26", "设置预览分辨率 width:" + size.width + " height:" + size.height)
    }

    private fun rotation90(data: ByteArray) {
        var index = 0
        val ySize = mWidth * mHeight
        //u和v
        val uvHeight = mHeight / 2
        //后置摄像头顺时针旋转90度
        if (mCameraId == CameraInfo.CAMERA_FACING_BACK) {
            //将y的数据旋转之后 放入新的byte数组
            for (i in 0 until mWidth) {
                for (j in mHeight - 1 downTo 0) {
                    bytes!![index++] = data[mWidth * j + i]
                }
            }

            //每次处理两个数据
            var i = 0
            while (i < mWidth) {
                for (j in uvHeight - 1 downTo 0) {
                    // v
                    bytes!![index++] = data[ySize + mWidth * j + i]
                    // u
                    bytes!![index++] = data[ySize + mWidth * j + i + 1]
                }
                i += 2
            }
        } else {
            //逆时针旋转90度
            for (i in 0 until mWidth) {
                var nPos = mWidth - 1
                for (j in 0 until mHeight) {
                    bytes!![index++] = data[nPos - i]
                    nPos += mWidth
                }
            }
            //u v
            var i = 0
            while (i < mWidth) {
                var nPos = ySize + mWidth - 1
                for (j in 0 until uvHeight) {
                    bytes!![index++] = data[nPos - i - 1]
                    bytes!![index++] = data[nPos - i]
                    nPos += mWidth
                }
                i += 2
            }
        }
    }


    interface OnChangedSizeListener {
        fun onChanged(w: Int, h: Int)
    }
}