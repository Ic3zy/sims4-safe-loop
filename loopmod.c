// loopmod.c
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <Windows.h>

typedef struct
{
    PyObject_HEAD PyObject *func;
    PyObject *interval_cb;
    volatile LONG running;
    HANDLE thread;
    DWORD interval_ms;
} LoopObject;

static int loop_tick(void *arg)
{
    LoopObject *self = (LoopObject *)arg;

    if (!self->running)
        return 0;

    // func()
    PyObject *res = PyObject_CallObject(self->func, NULL);
    if (!res)
    {
        PyErr_Print();
    }
    else
    {
        Py_DECREF(res);
    }

    PyObject *ival = PyObject_CallObject(self->interval_cb, NULL);
    if (!ival)
    {
        PyErr_Print();
        return 0;
    }

    double sec = PyFloat_AsDouble(ival);
    Py_DECREF(ival);

    if (PyErr_Occurred())
    {
        PyErr_Print();
        return 0;
    }

    if (sec <= 0.0)
    {
        sec = 0.001;
    }

    self->interval_ms = (DWORD)(sec * 1000.0);

    return 0;
}

static DWORD WINAPI loop_thread_proc(LPVOID arg)
{
    LoopObject *self = (LoopObject *)arg;

    if (self->running)
    {
        if (Py_AddPendingCall(loop_tick, self) != 0)
        {
            return 0;
        }
    }

    while (self->running)
    {
        DWORD ms = self->interval_ms;
        if (ms == 0)
        {
            ms = 1;
        }

        Sleep(ms);

        if (!self->running)
            break;

        if (Py_AddPendingCall(loop_tick, self) != 0)
        {
            Sleep(1);
        }
    }

    return 0;
}

// --------------------------------------------------------
// Loop.start()
// --------------------------------------------------------
static PyObject *Loop_start(LoopObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->running)
    {
        Py_RETURN_NONE;
    }

    if (!self->func || !self->interval_cb)
    {
        PyErr_SetString(PyExc_RuntimeError, "Loop is not properly initialized");
        return NULL;
    }

    self->interval_ms = 1;
    self->running = 1;

    HANDLE h = CreateThread(NULL, 0, loop_thread_proc, self, 0, NULL);
    if (!h)
    {
        self->running = 0;
        PyErr_SetString(PyExc_RuntimeError, "Failed to create thread");
        return NULL;
    }

    self->thread = h;
    Py_RETURN_NONE;
}

// --------------------------------------------------------
// Loop.stop()
// --------------------------------------------------------
static PyObject *Loop_stop(LoopObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!self->running)
    {
        Py_RETURN_NONE;
    }

    self->running = 0;

    if (self->thread)
    {
        WaitForSingleObject(self->thread, INFINITE);
        CloseHandle(self->thread);
        self->thread = NULL;
    }

    Py_RETURN_NONE;
}

// --------------------------------------------------------
// Dealloc
// --------------------------------------------------------
static void Loop_dealloc(LoopObject *self)
{
    if (self->running)
    {
        self->running = 0;
        if (self->thread)
        {
            WaitForSingleObject(self->thread, INFINITE);
            CloseHandle(self->thread);
            self->thread = NULL;
        }
    }

    Py_XDECREF(self->func);
    Py_XDECREF(self->interval_cb);

    Py_TYPE(self)->tp_free((PyObject *)self);
}

// --------------------------------------------------------
// __new__
// --------------------------------------------------------
static PyObject *Loop_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LoopObject *self = (LoopObject *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;

    self->func = NULL;
    self->interval_cb = NULL;
    self->running = 0;
    self->thread = NULL;
    self->interval_ms = 1;

    return (PyObject *)self;
}

// --------------------------------------------------------
// __init__(self, func, interval_callable)
// --------------------------------------------------------
static int Loop_init(LoopObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *func;
    PyObject *interval;

    if (!PyArg_ParseTuple(args, "OO", &func, &interval))
    {
        return -1;
    }

    if (!PyCallable_Check(func))
    {
        PyErr_SetString(PyExc_TypeError, "func must be callable");
        return -1;
    }

    if (!PyCallable_Check(interval))
    {
        PyErr_SetString(PyExc_TypeError, "interval must be callable");
        return -1;
    }

    Py_INCREF(func);
    Py_INCREF(interval);

    Py_XDECREF(self->func);
    Py_XDECREF(self->interval_cb);

    self->func = func;
    self->interval_cb = interval;

    self->interval_ms = 1;
    self->running = 0;

    return 0;
}

static PyMethodDef Loop_methods[] = {
    {"start", (PyCFunction)Loop_start, METH_NOARGS, "Start the loop"},
    {"stop", (PyCFunction)Loop_stop, METH_NOARGS, "Stop the loop"},
    {NULL}};

static PyTypeObject LoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "loopmod.Loop",
    .tp_basicsize = sizeof(LoopObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Loop_new,
    .tp_init = (initproc)Loop_init,
    .tp_dealloc = (destructor)Loop_dealloc,
    .tp_methods = Loop_methods,
};

static struct PyModuleDef loopmod_module = {
    PyModuleDef_HEAD_INIT,
    "loopmod",
    "Simple main-thread loop with background sleep",
    -1,
};

PyMODINIT_FUNC PyInit_loopmod(void)
{
    PyObject *m;

    if (PyType_Ready(&LoopType) < 0)
        return NULL;

    m = PyModule_Create(&loopmod_module);
    if (!m)
        return NULL;

    Py_INCREF(&LoopType);
    if (PyModule_AddObject(m, "Loop", (PyObject *)&LoopType) < 0)
    {
        Py_DECREF(&LoopType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
