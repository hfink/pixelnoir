//Copyright (c) 2010 Heinrich Fink <hf (at) hfink (dot) eu>, 
//                   Thomas Weber <weber (dot) t (at) gmx (dot) at>
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

#ifndef ANIMEVALUATOR_H
#define ANIMEVALUATOR_H

#include "common.h"

#include "type_info.h"

namespace rtr_format {
    class Animation;
    class Animation_Channel;
    class Animation_Sampler;
}

using rtr_format::Animation;
using rtr_format::Animation_Channel;
using rtr_format::Animation_Sampler;

template<typename T> class AnimListener;

/**
 * Animation runtime class
 * Handles animation evaluation and signal listeners.
 */
class AnimEvaluator 
{
    template<typename T> friend class AnimListener;

    public:
 
    AnimEvaluator() : _time(0.0) {};

    /**
     * Add rtr_format::Animation object to evaluator.
     * @param animation The animation.
     * @param time_offset Optional time offset.
     */
    void add_animation (const Animation& animation, float time_offset = 0.0);
    
    /**
     * Remove animation.
     * @param animation_id Name of the animation.
     */
    void remove_animation (const string& animation_id);
    
    /**
     * Increment time and evaluate all animations.
     * @param time_diff Time difference.
     */
    void update (float time_diff);

    /**
     * Set timer and evaluate all animations.
     * @param time Time.
     */
    void update_absolute(float time);

    private:

    shared_array<float> get_listener_ref(const string& name,
                                         int components);

    shared_array<float> get_listener_ref(const string& name,
                                         int& out_offset,
                                         int components);
    /**
     * Return listener.
     * AnimListener's destructor will call this for you!
     * @param name Listener name.
     */
    void free_listener(const string& name);


    struct AnimEntry {
        struct ChannelEntry {
            ChannelEntry(const Animation& animation,
                         const Animation_Channel& channel,
                         AnimEvaluator* parent);

            void free_listeners(AnimEvaluator* parent);
            void update(float time);

            const Animation_Sampler* _time_sampler;
            const Animation_Sampler* _data_sampler;

            int _components;
            int _target_offset;

            string _target_name;
            shared_array<float> _target_ref;

            float _start_time, _end_time;
            int _current_segment;
        };

        AnimEntry(const Animation& animation, float time_offset,
                  AnimEvaluator* parent);
        AnimEntry(const AnimEntry& init);
        AnimEntry() : _animation(NULL), _parent(NULL) {};
        ~AnimEntry();

        void update(float time);
        void operator= (const AnimEntry& init);

        const Animation* _animation;
        mutable list<ChannelEntry> _channels;
        float _time_offset;
        AnimEvaluator* _parent;
    };

    struct ListenerEntry {
        shared_array<float> values;
        int components;
        int use_count;
    };

    map<string, ListenerEntry> _listeners;
    map<string, AnimEntry> _animations;
    float _time;
};

/**
 * Statically typed wrapper class for animation-listeners.
 */
template<typename T>
class AnimListener
{
    AnimEvaluator& _parent; /**< Source of listener */
    shared_array<float> _values; /**< The animated data */
    string _name; /**< Name of listener object */


    public:

    /**
     * @param parent Pointer to the parent AnimEvaluator class.
     * @param name Name of the listener.
     */
    AnimListener(AnimEvaluator& parent,
                 const string& name) :
        _parent(parent), 
        _values(parent.get_listener_ref(name, 
                                        gltype_info<T>::components)), 
        _name(name) {};

    /**
     * Copy constructor. Swaps ownership.
     */
    AnimListener(const AnimListener<T>& init) :
        _parent(init._parent), 
        _values(init._parent.get_listener_ref(init.name, 
                                              gltype_info<T>::components)),
        _name(init._name) {}
    ~AnimListener();

    /**
     * Statically typed setter function.
     */
    void set(const T& in) 
    {
        gltype_info<T>::set_float_array(in, _values.get());
    }

    /**
     * Statically typed getter function.
     */
    T value() const
    {
        return gltype_info<T>::build_from_floats(_values.get());
    }
};

template <typename T>
    AnimListener<T>::~AnimListener()
{
    _parent.free_listener(_name);
}


#endif
