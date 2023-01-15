package com.example.avstudydemo2.player;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.Toast;
import com.example.avstudydemo2.R;

public class DemoPlayerActivity extends AppCompatActivity {

    private DemoPlayer player;
    public String url = "http://live.cgtn.com/500/prog_index.m3u8";
//    public String url = "http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8";
//    public String url = "http://liveop.cctv.cn/hls/4KHD/playlist.m3u8";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager
                .LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_demo_player);

        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        player = new DemoPlayer();
        player.setDataSource(url);
        player.setSurfaceView(surfaceView);
        player.setOnPrepareListener(new DemoPlayer.OnPrepareListener() {
            @Override
            public void onPrepare() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(DemoPlayerActivity.this, "开始播放", Toast.LENGTH_SHORT).show();
                    }
                });
                player.start();
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        player.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        player.release();
    }
}