package com.example.hookso;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.example.hookso.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'hookso' library on application startup.
    static {
        System.loadLibrary("hook");
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

        String classname = retclassname(Runtime.class);
    }

    /**
     * A native method that is implemented by the 'hookso' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public native String retclassname(Class name);
}