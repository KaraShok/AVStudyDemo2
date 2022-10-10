package com.example.avstudydemo2;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.example.avstudydemo2.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'avstudydemo2' library on application startup.
    static {
        System.loadLibrary("avstudydemo2");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    /**
     * A native method that is implemented by the 'avstudydemo2' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}