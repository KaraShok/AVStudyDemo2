package com.example.avstudydemo2.player;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * @author zhangyaozhong @ Zhihu Inc.
 * @since 10-11-2022
 */
public class DemoPlayer implements SurfaceHolder.Callback {

    private String dataSource;
    private SurfaceHolder holder;
    private OnPrepareListener listener;


    /**
     * 让使用 设置播放的文件 或者 直播地址
     */
    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 设置播放显示的画布
     *
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    public void onError(int errorCode){
        System.out.println("Java接到回调:"+errorCode);
    }


    public void onPrepare(){
        if (null != listener){
            listener.onPrepare();
        }
    }

    public void setOnPrepareListener(OnPrepareListener listener){
        this.listener = listener;
    }
    public interface OnPrepareListener{
        void onPrepare();
    }

    /**
     * 准备好 要播放的视频
     */
    public void prepare() {
        nativePrepare(dataSource);
    }

    /**
     * 开始播放
     */
    public void start() {
        nativeStart();
    }

    /**
     * 停止播放
     */
    public void stop() {
        nativeStop();
    }

    public void release() {
        holder.removeCallback(this);
        nativeRelease();
    }

    /**
     * 画布创建好了
     *
     * @param holder
     */
    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    /**
     * 画布发生了变化（横竖屏切换、按了home都会回调这个函数）
     *
     * @param holder
     * @param format
     * @param width
     * @param height
     */
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        nativeSetSurface(holder.getSurface());
    }

    /**
     * 销毁画布 (按了home/退出应用/)
     *
     * @param holder
     */
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }


    native void nativePrepare(String dataSource);

    native void nativeStart();

    native void nativeStop();

    native void nativeRelease();

    native void nativeSetSurface(Surface surface);
}
