package com.zj26.pushdemo

import android.Manifest
import android.hardware.Camera
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import androidx.core.content.PermissionChecker
import com.zj26.pushdemo.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var livePusher: LivePusher

    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {
            if (it[Manifest.permission.CAMERA] == true) {
                livePusher.setPreviewDisplay(binding.surfaceView.holder)
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        livePusher = LivePusher(
            this,
            800,
            400,
            800_000,
            10,
            Camera.CameraInfo.CAMERA_FACING_BACK
        )
        val permissions = arrayOf(Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO)
        val camera = ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
        val recordAudio = ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
        if (camera == PermissionChecker.PERMISSION_GRANTED && recordAudio == PermissionChecker.PERMISSION_GRANTED) {
            livePusher.setPreviewDisplay(binding.surfaceView.holder)
        } else {
            requestPermissionLauncher.launch(permissions)
        }
        lifecycle
    }

    fun start(view: View) {
        livePusher.startLive()
    }
    fun stop(view: View) {}
    fun switchCamera(view: View) {
        livePusher.switchCamera()
    }
}