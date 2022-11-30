package com.example.jjpeajar.jors_1;

import androidx.appcompat.app.AppCompatActivity;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Adapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.VideoView;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.UUID;

public class HomeActivity extends AppCompatActivity {


    private static final String TAG = "MainActivity";


    private TextView txNVel;
    private TextView txNAc;
    private ImageView btForward;
    private ImageView btBackward;
    private ImageView btLeft;
    private ImageView btRight;
    private ImageView btPlayPause;
    private Button buttonStream;
    private VideoView videoView;
    private Button btSt;
    private Button btConn;
    private SeekBar seekBarVel;
    private SeekBar seekBarAc;


    private float vel;
    private float ac;
    private float velTx;
    private float acTx;
    private boolean stop;
    private int dir;

    private BluetoothSocket btSocket;

    private boolean connection;

    ProgressDialog dialog;
    String videoUrl = "rstp://192.168.XXX.XXX/live/live";
    BluetoothAdapter btAdapter;
    private static final UUID MY_UUID_INSECURE =
            UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");


    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy: called.");
        super.onDestroy();
        try {
            btSocket.close();
            System.out.println(btSocket.isConnected());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);



        btBackward=findViewById(R.id.btBackward);
        btForward=findViewById(R.id.btForward);
        btLeft=findViewById(R.id.btLeft);
        btRight=findViewById(R.id.btRight);
        btPlayPause=findViewById(R.id.btPlayPause);
        btSt=findViewById(R.id.btSt);
        btConn=findViewById(R.id.btConn);
        txNVel=findViewById(R.id.txNVel);
        txNAc=findViewById(R.id.txNAc);
        videoView=findViewById(R.id.videoView);


        seekBarVel=findViewById(R.id.seekBarVel);
        seekBarAc=findViewById(R.id.seekBarAc);

        ac=0.1f;
        vel=1f;

        acTx=ac*100;
        velTx=vel*10;

        seekBarVel.setProgress((int)velTx);
        seekBarAc.setProgress((int)acTx);

        txNVel.setText(String.valueOf(ac));
        txNAc.setText(String.valueOf(vel));

        btPlayPause.setOnClickListener(this::onClick);


        btConn.setVisibility(View.INVISIBLE);



        btAdapter = BluetoothAdapter.getDefaultAdapter();
        System.out.println(btAdapter.getBondedDevices());

        BluetoothDevice hc05 = btAdapter.getRemoteDevice("0C:DC:7E:CC:47:62");
        try {
            if (hc05.getName().equals("Videocoche")) {
                btConn.setVisibility(View.VISIBLE);
            }
            System.out.println(hc05.getName());
        } catch (Exception e) {
            e.printStackTrace();
        }

        btSocket = null;

        btForward.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dir=1;
                MandarMensajeBt("Ve1:", vel);
            }
        });
        btBackward.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dir=-1;
                MandarMensajeBt("Ve-1:", vel);
            }
        });

        btLeft.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dir=2;
                MandarMensajeBt("Ve"+ String.valueOf(vel) + ":", 2);
            }
        });

        btRight.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                dir=3;
                MandarMensajeBt("Ve"+ String.valueOf(vel) + ":", 3);
            }
        });

        btConn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Log.d(TAG, "Trying to pair with videocoche");
                int contador = 0;
                do {
                    try {
                        btSocket = hc05.createRfcommSocketToServiceRecord(MY_UUID_INSECURE);//
                        System.out.println(btSocket);
                        btSocket.connect();
                        System.out.println(btSocket.isConnected());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    contador++;
                } while (!btSocket.isConnected() && contador < 3);

                connection=btSocket.isConnected();
            }
        });

        if (stop){
            btSt.setText("OFF");
        }else{
            btSt.setText("ON");
        }

        btSt.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if(stop){
                    stop = false;
                    MandarMensajeBt("St", 0);
                }else{
                    stop = true;
                    MandarMensajeBt("St", 1);
                }
            }
        });


        seekBarAc.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            float progressChangedValue = 0;

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChangedValue = (float)progress/100;
                ac=progressChangedValue;

                txNAc.setText(String.valueOf(ac));



            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                MandarMensajeBt("Ac", progressChangedValue);
            }
        });
        seekBarVel.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            float progressChangedValue = 0;

            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                progressChangedValue = (float)progress/10;
                vel=progressChangedValue;

                txNVel.setText(String.valueOf(vel));

            }

            public void onStartTrackingTouch(SeekBar seekBar) {
                // TODO Auto-generated method stub
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
                vel=progressChangedValue;
                if(!stop){
                    MandarMensajeBt("Ve" + String.valueOf(dir) + ":", vel);
                }
            }
        });

    }

    public void onClick(View v){
        dialog=new ProgressDialog(HomeActivity.this);
        dialog.setMessage("Please wait... ");
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();

        try {
            if (videoView.isPlaying()){
                Uri uri = Uri.parse(videoUrl);
                videoView.setVideoURI(uri);
                videoView.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    @Override
                    public void onCompletion(MediaPlayer mediaPlayer) {
                        btPlayPause.setImageResource(R.drawable.play60);

                    }
                });
            }else{
                videoView.pause();
                btPlayPause.setImageResource(R.drawable.pausa64);
            }

        }catch (Exception ex){

        }
        videoView.requestFocus();
        videoView.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
            @Override
            public void onPrepared(MediaPlayer mediaPlayer) {
                dialog.dismiss();
                videoView.start();
            }
        });
    }

    private void MandarMensajeBt(String prefijo, float valor){
        String msg = prefijo + String.valueOf(valor);
        if(connection){
            try {
                OutputStream outputStream = btSocket.getOutputStream();
                outputStream.write(msg.getBytes(StandardCharsets.UTF_8));
            } catch (IOException e) {
                e.printStackTrace();
            }

        }else{
            Log.d(TAG, "NO HAY CONEXION CON VIDEOCOCHE");
        }
    }
    private void MandarMensajeBt(String prefijo, int valor){
        String msg = prefijo + String.valueOf(valor);
        if(connection){
            try {
                OutputStream outputStream = btSocket.getOutputStream();
                outputStream.write(msg.getBytes(StandardCharsets.UTF_8));
            } catch (IOException e) {
                e.printStackTrace();
            }

        }else{
            Log.d(TAG, "NO HAY CONEXION CON VIDEOCOCHE");
        }
    }


}