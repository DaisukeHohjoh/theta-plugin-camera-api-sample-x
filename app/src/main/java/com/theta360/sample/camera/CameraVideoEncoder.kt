package com.theta360.sample.camera

import android.media.MediaCodec
import android.media.MediaCodec.CodecException
import android.media.MediaFormat
import android.media.MediaMuxer
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import java.io.IOException
import java.util.concurrent.ArrayBlockingQueue
import java.util.concurrent.TimeUnit

class CameraVideoEncoder(
    private val mEncoderMimeType: String,
    private val mMediaFormat: MediaFormat,
    private val id: Int,
    cb: returnBufferCallback?,
    filename: String
) {
    private var mMediaCodec: MediaCodec?
    private var mMediaMuxer: MediaMuxer?
    private var mCodecThread: HandlerThread? = null
    private var mCodecHandler: Handler? = null
    private val mBufQueue: ArrayBlockingQueue<FrameBuffer?>? = ArrayBlockingQueue<FrameBuffer?>(100, true)
    private val mBufCb: returnBufferCallback?
    private val mMuxerFormat = MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4
    private var mMuxerVideoTrack = 0
    private var accTime: Long = 0
    private var lastTs: Long = 0
    private var mState: CodecState

    private enum class CodecState {
        Ready, Running, Stopped, Finished
    }

    interface returnBufferCallback {
        fun onBufferDone(data: ByteArray?)
    }

    fun start() {
        Log.d(TAG, "start")
        mMediaCodec!!.start()
        mState = CodecState.Running
    }

    fun stop() {
        Log.d(TAG, "stop")
        mState = CodecState.Stopped
    }

    fun addFrameBuffer(frameBuffer: FrameBuffer?) {
        try {
            mBufQueue!!.put(frameBuffer)
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    private fun createCodec(mimeType: String, mediaFormat: MediaFormat): MediaCodec {
        try {
            Log.d(TAG, "Create MediaCodec thread")
            val mediaCodec = MediaCodec.createEncoderByType(mimeType)
            mCodecThread = HandlerThread("CodecThread$id")
            mCodecThread!!.start()
            mCodecHandler = Handler(mCodecThread!!.looper)
            mediaCodec.setCallback(object : MediaCodec.Callback() {
                var frameIndex = 1
                override fun onInputBufferAvailable(codec: MediaCodec, inputBufferId: Int) {
                    Log.d(
                        TAG,
                        "<" + id + "_onInputBufferAvailable start>" + "<inputBufferId>" + inputBufferId + "<frameIndex>" + frameIndex + "State" + mState
                    )
                    var buff: FrameBuffer? = null
                    synchronized(mBufQueue!!) {
                        try {
                            buff = mBufQueue.poll(
                                BUFF_QUEUE_TIMEOUT.toLong(),
                                TimeUnit.MILLISECONDS
                            )
                        } catch (e: InterruptedException) {
                            e.printStackTrace()
                        }
                        if (buff == null) {
                            if (mState == CodecState.Stopped) {
                                // Send EOS flag to encoder
                                Log.d(
                                    TAG,
                                    "<" + id + "_onInputBufferAvailable end> end of stream "
                                )
                                mMediaCodec!!.queueInputBuffer(
                                    inputBufferId,
                                    0,
                                    0,
                                    0,
                                    MediaCodec.BUFFER_FLAG_END_OF_STREAM
                                )
                            } else {
                                Log.e(
                                    TAG,
                                    "MediaCodec[$id]:dequeue frame $frameIndex timeout"
                                )
                            }
                            return
                        }
                    }
                    val size: Int = buff!!.size
                    val frameId: Int = buff!!.frameId
                    val offset = size * id
                    val inputBuffer = codec.getInputBuffer(inputBufferId)
                    inputBuffer.clear()
                    Log.d(
                        TAG,
                        "<" + id + "_onInputBufferAvailable end> inputBuffer.put <offset>=" + offset + "<size>" + size
                    )
                    inputBuffer.put(buff?.data, offset, size)
                    ++buff!!.copied
                    if (buff!!.copied >= 2) {
                        // Buffer is copied for both encoders, return it to camera pipeline
                        if (mBufCb != null) {
                            Log.d(
                                TAG,
                                "<" + id + "_onInputBufferAvailable onBufferDone <buff.copied>" + buff!!.copied
                            )
                            mBufCb.onBufferDone(buff?.data)
                        }
                    }
                    val pts = computePresentationTime(frameId.toLong(), buff!!.timestamp)
                    mMediaCodec!!.queueInputBuffer(inputBufferId, 0, size, pts, 0)
                    Log.d(
                        TAG,
                        "<" + id + "_onInputBufferAvailable end>" + "queueInputBuffer <size>" + size + "<frameId>" + frameId + "<pts>" + pts
                    )
                }

                override fun onOutputBufferAvailable(
                    codec: MediaCodec,
                    outputBufferId: Int,
                    bufferInfo: MediaCodec.BufferInfo
                ) {
                    Log.d(
                        TAG,
                        "<" + id + "_onOutputBufferAvailable start>" + "<outputBufferId>" + outputBufferId + "<frameIndex>" + frameIndex
                    )
                    val outputBuffer = codec.getOutputBuffer(outputBufferId)
                    if (bufferInfo.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0) {

                        // Get the codec config and provide it for MediaMuxer configuration.
                        val format = mMediaCodec!!.outputFormat
                        format.setByteBuffer("csd-0", outputBuffer)
                        format.setInteger("id", id)
                        mMuxerVideoTrack = addMuxerTrack(format)
                        startMediaMuxer()
                        Log.d(TAG, "<" + id + "_onOutputBufferAvailable end> startMediaMuxer")
                        mMediaCodec!!.releaseOutputBuffer(outputBufferId, false)
                        return
                    } else if (bufferInfo.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) {
                        Log.d(TAG, "<" + id + "_onOutputBufferAvailable end> stopEncoding")
                        stopEncoding()
                        return
                    }
                    mMediaMuxer!!.writeSampleData(mMuxerVideoTrack, outputBuffer, bufferInfo)
                    mMediaCodec!!.releaseOutputBuffer(outputBufferId, false)
                    Log.d(TAG, "<" + id + "_onOutputBufferAvailable end> writeSampleData")
                    frameIndex++
                }

                override fun onOutputFormatChanged(mc: MediaCodec, format: MediaFormat) {}
                override fun onError(codec: MediaCodec, exception: CodecException) {
                    Log.e(
                        TAG,
                        "MediaCodec exception $exception"
                    )
                    exception.printStackTrace()
                }
            }, mCodecHandler)
            mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            return mediaCodec
        } catch (e: IOException) {
            Log.e(TAG, "Error while creating Media Codec")
            e.printStackTrace()
        }
        throw RuntimeException("Could not create Media Codec")
    }

    private fun createMuxer(muxerFormat: Int, filename: String): MediaMuxer {
        try {
            return MediaMuxer(filename, muxerFormat)
        } catch (e: IOException) {
            Log.e(TAG, "Error while creating Media Muxer")
            e.printStackTrace()
        }
        throw RuntimeException("Could not create Media Muxer")
    }

    private fun startMediaMuxer() {
        mMediaMuxer!!.start()
    }

    private fun addMuxerTrack(mediaFormat: MediaFormat): Int {
        return mMediaMuxer!!.addTrack(mediaFormat)
    }

    private fun computePresentationTime(frameIndex: Long, timestamp: Long): Long {
        var presentTime: Long = 0
        if (frameIndex == 1L) {
            accTime = 0
            lastTs = timestamp
            return presentTime
        }
        presentTime = (timestamp - lastTs) / 1000
        lastTs = timestamp
        if (presentTime == 0L) {
            // frame is duplicated, this should not happen
            val fps = mMediaFormat.getInteger(MediaFormat.KEY_FRAME_RATE)
            accTime += (1000000 / fps).toLong()
        }
        accTime += presentTime
        return accTime
    }

    private fun stopEncoding() {
        Log.v(TAG, "MediaCodec[$id]: stopEncoding E")
        mState = CodecState.Stopped
        if (mMediaMuxer != null) {
            mMediaMuxer!!.stop()
            mMediaMuxer!!.release()
            mMediaMuxer = null
        }
        if (mMediaCodec != null) {
            mMediaCodec!!.stop()
            mMediaCodec!!.release()
            mMediaCodec = null
        }
        mMuxerVideoTrack = -1
        mBufQueue?.clear()
        if (mCodecThread != null) {
            mCodecThread!!.quitSafely()
            mCodecThread = null
            mCodecHandler = null
        }
        Log.v(TAG, "MediaCodec[$id]: stopEncoding X")
        mState = CodecState.Finished
    }

    val isFinish: Boolean
        get() {
            var ret = false
            if (mState == CodecState.Finished) {
                ret = true
                Log.d(TAG, "isFinish : true")
            }
            return ret
        }

    companion object {
        //private const val TAG = "CAM_VideoEncoder"
        private const val TAG = "Camera_API_Sample"
        private const val BUFF_QUEUE_TIMEOUT = 1000
    }

    init {
        mMediaCodec = createCodec(mEncoderMimeType, mMediaFormat)
        mMediaMuxer = createMuxer(mMuxerFormat, filename)
        mState = CodecState.Ready
        mBufCb = cb
    }
}

class FrameBuffer(var data: ByteArray, var frameId: Int, var size: Int, var timestamp: Long) {
    var offset = 0
    var copied = 0
}
