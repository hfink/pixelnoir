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

#include "rtr_format.pb.h"
#include "AnimEvaluator.h"
#include <boost/regex.hpp>
#include "roots.h"
#include <math.h>

void AnimEvaluator::add_animation(const Animation& animation, float time_offset)
{
    if (_animations.count(animation.id()) > 0)
        return;

    AnimEntry entry = AnimEntry(animation, time_offset, this);
    _animations[animation.id()] = entry;
}

void AnimEvaluator::remove_animation(const string& animation_id)
{
    _animations.erase(animation_id);
}

void AnimEvaluator::free_listener(const string& name)
{
    if (_listeners.count(name) <= 0)
        return;

    ListenerEntry& listener = _listeners[name];

    listener.use_count--;

    if (listener.use_count <= 0) {
        _listeners.erase(name);
    }
}

void AnimEvaluator::update (float time_diff)
{
    update_absolute(_time + time_diff);
    
}

void AnimEvaluator::update_absolute(float time)
{
    _time = time;
    
    for (map<string, AnimEntry>::iterator i = _animations.begin();
         i != _animations.end(); ++i) {
        i->second.update(time);
    }
}

shared_array<float> AnimEvaluator::get_listener_ref(const string& name,
                                                    int components)
{
    if (_listeners.count(name) > 0) {
        ListenerEntry& listener = _listeners[name];
        assert(listener.components == components);
        listener.use_count++;
        return listener.values;
    }

    ListenerEntry listener = {shared_array<float>(new float[components]),
                              components, 1};
    _listeners[name] = listener;
    return listener.values;
}

shared_array<float> AnimEvaluator::get_listener_ref(const string& name,
                                                    int& out_offset,
                                                    int components)
{
    const boost::regex pattern("([A-Za-z0-9\\-_/]+)(\\.(.+))?");
    const boost::regex pattern_x("[XRSU]"); // Patterns for first coordinate
    const boost::regex pattern_y("[YGTV]"); // Patterns for second coord
    const boost::regex pattern_z("[ZBP]");  // ..third coordinate
    const boost::regex pattern_w("[WAQ]");  // ..fourth

    boost::match_results<std::string::const_iterator> match;

    if (!boost::regex_match(name, match, pattern)) {
        cerr << "Error AnimEntry::get_listener_ref: "
             << "Listener name \"" << name << "\" could not be matched" << endl;
        return shared_array<float>(new float[components]);
    }

    if (_listeners.count(match[1]) <= 0) {
        cerr << "Error AnimEntry::get_listener_ref: "
             << "Could not find listener \"" << match[1] << "\"" << endl;
        return shared_array<float>(new float[components]);
    }

    ListenerEntry& entry = _listeners[match[1]];
    out_offset = 0;

    if (match[3] != "") {
        string m(match[3]);
        if        (boost::regex_match(m, pattern_x)) {
            out_offset = 0;
        } else if (boost::regex_match(m, pattern_y)) {
            out_offset = 1;
        } else if (boost::regex_match(m, pattern_z)) {
            out_offset = 2;
        } else if (boost::regex_match(m, pattern_w)) {
            out_offset = 3;
        } else {
            cerr << "Error AnimEntry::get_listener_ref: "
                 << "The listener subscript of \"" << name << "\" "
                 << "with component size " << entry.components 
                 << " does not match." << endl;
            return shared_array<float>(new float[components]);
        }
    }

    if (out_offset+components > entry.components) {
        cerr << "Error AnimEntry::get_listener_ref: "
             << "Listener size mismatch"
             << " components=" << components
             << ", offset=" << out_offset 
             << ", entry_components=" << entry.components << endl;
        return shared_array<float>(new float[components+out_offset]);
    }

    return entry.values;
}

AnimEvaluator::AnimEntry::AnimEntry(const Animation& animation,
                         float time_offset, AnimEvaluator* parent) :
    _animation(&animation), _channels(), _time_offset(time_offset), _parent(parent)
{
    for (int i = 0; i < animation.channel_size(); ++i) {
        _channels.push_back(ChannelEntry(animation, 
                                         animation.channel(i), 
                                         parent));
    }
}

AnimEvaluator::AnimEntry::AnimEntry(const AnimEntry& init) :
    _animation(init._animation),
    _channels(),
    _time_offset(init._time_offset),
    _parent(init._parent)
{
    _channels.swap(init._channels);
}

void AnimEvaluator::AnimEntry::operator=(const AnimEntry& init) 
{
    _animation = init._animation;
    _time_offset = init._time_offset;
    _parent = init._parent;

    _channels.swap(init._channels);
}

AnimEvaluator::AnimEntry::~AnimEntry()
{
    for (list<ChannelEntry>::iterator i = _channels.begin();
         i != _channels.end(); ++i) {
        i->free_listeners(_parent);
    }
}

void AnimEvaluator::AnimEntry::update(float time)
{
    for (list<ChannelEntry>::iterator i = _channels.begin();
         i != _channels.end(); ++i) {
        i->update(time+_time_offset);
    }
}

AnimEvaluator::AnimEntry::ChannelEntry::ChannelEntry
    (const Animation& animation,
     const Animation_Channel& channel,
     AnimEvaluator* parent) :
    _time_sampler(NULL), _data_sampler(NULL),
    _target_name(channel.target()),
    _current_segment(0)
{
    for (int i = 0; i < animation.sampler_size(); ++i) {
        if (animation.sampler(i).id() == channel.time_sampler()) {
            _time_sampler = &(animation.sampler(i));
        }

        if (animation.sampler(i).id() == channel.data_sampler()) {
            _data_sampler = &(animation.sampler(i));
        }
    }

    if (_time_sampler == NULL) {
        cerr << "Error ChannelEntry constructor: "
             << "Could not find time sampler \""
             << channel.time_sampler() << "\"" << endl;
        return;
    }

    if (_data_sampler == NULL) {
        cerr << "Error ChannelEntry constructor: "
             << "Could not find data sampler \""
             << channel.data_sampler() << "\"" << endl;
        return;
    }

    if (_time_sampler->components() != 1) {
        cerr << "Error ChannelEntry constructor: "
             << "Time sampler \"" << _time_sampler->id() 
             << "\" has to be 1-dimensional"
             << endl;
    }

    assert(_time_sampler->control_point_size() == 
           (_time_sampler->segment_count()*3+1) * _time_sampler->components());
    assert(_data_sampler->control_point_size() == 
           (_data_sampler->segment_count()*3+1) * _data_sampler->components());
    assert(_data_sampler->segment_count() == _time_sampler->segment_count());

    _components = _data_sampler->components();
    
    _target_ref = parent->get_listener_ref(_target_name, _target_offset, 
                                           _components);    
    _start_time = _time_sampler->control_point(0);
    _end_time = _time_sampler->control_point(_time_sampler->segment_count()*3);
}

void AnimEvaluator::AnimEntry::ChannelEntry::free_listeners(AnimEvaluator* parent)
{
    if (_target_name != "")
        parent->free_listener(_target_name);
    _target_ref.reset();
}


bool find_zero (float X1, float X2, float X3, float X4, float x, float& t)
{
    float c0,c1,c2,c3;
    float ts[3];

    // Dealing with border cases. We have to be a little more tolerant here
    // due to possible numerical issues
    static const float EPSILON = 0.0001f;
    if ( abs(x - X1) < EPSILON ) {
        t = 0.0f;
        return true;
    } else if (abs(x - X4) < EPSILON) {
        t = 1.0f;
        return true;
    }

    c0= X1 - x;
    c1= 3.0f * (X2 - X1);
    c2= 3.0f * (X1 - 2.0f*X2 + X3);
    c3= X4 - X1 + 3.0f * (X2 - X3);
    
    int solutions = solveCubic(c3,c2,c1,c0,ts);

    if        (solutions > 2 && ts[2] >= 0.0f && ts[2] <= 1.0f) {
        t = ts[2];
        return true;
    } else if (solutions > 1 && ts[1] >= 0.0f && ts[1] <= 1.0f) {
        t = ts[1];
        return true;
    } else if (solutions > 0 && ts[0] >= 0.0f && ts[0] <= 1.0f) {
        t = ts[0];
        return true;
    }

    return false;
}

float eval_bezier (float f1, float f2, float f3, float f4, float t);

void AnimEvaluator::AnimEntry::ChannelEntry::update(float time)
{
    
    float local_time = time;
    
    // Clamp if time is outside of [start_time, end_time]
    if (time <= _start_time) {
        for (int i = 0; i < _components; ++i) {
            // Offset of first point in curve for component i
            int offset = (_data_sampler->segment_count()*3+1)*i;
            _target_ref[i+_target_offset] = _data_sampler->control_point(offset);
        }
        return;
    }

    if (time >= _end_time) {
        for (int i = 0; i < _components; ++i) {
            // Offset of last point in curve for component i
            int offset = (_data_sampler->segment_count()*3+1)*i;
            offset += _data_sampler->segment_count() * 3;
            _target_ref[i+_target_offset] = _data_sampler->control_point(offset);
        }
        return;
    }

    // Find current segment

    while (_current_segment < _time_sampler->segment_count() - 1 && 
           local_time >= _time_sampler->control_point(_current_segment*3 + 3)) {
        ++_current_segment;
    }

    while (_current_segment > 0 &&
           local_time <= _time_sampler->control_point(_current_segment*3)) {
        --_current_segment;
    }

    // Find t for our current time
    float X1 = _time_sampler->control_point(_current_segment * 3 + 0);
    float X2 = _time_sampler->control_point(_current_segment * 3 + 1);
    float X3 = _time_sampler->control_point(_current_segment * 3 + 2);
    float X4 = _time_sampler->control_point(_current_segment * 3 + 3);

    float t;
    bool success = find_zero(X1, X2, X3, X4, local_time, t);

    if (!success) {
        cerr << "Failed to invert time sampler spline" << endl;
        cerr << "x = " << local_time
             << ", X1 = " << X1
             << ", X2 = " << X2
             << ", X3 = " << X3
             << ", X4 = " << X4 
             << ", current_segment = " << _current_segment << endl;
        return;
    }

    // Evaluate
    for (int i = 0; i < _components; ++i) {
        int offset = (_data_sampler->segment_count()*3+1)*i;
        offset += _current_segment * 3;
        float Y1 = _data_sampler->control_point(offset + 0);
        float Y2 = _data_sampler->control_point(offset + 1);
        float Y3 = _data_sampler->control_point(offset + 2);
        float Y4 = _data_sampler->control_point(offset + 3);
        
        _target_ref[i + _target_offset] = eval_bezier(Y1, Y2, Y3, Y4, t);
    }
}

float eval_bezier (float P1, float P2, float P3, float P4, float t)
{
    float c0, c1, c2, c3;

    c0= P1;
    c1= 3.0f * (P2 - P1);
    c2= 3.0f * (P1 - 2.0f*P2 + P3);
    c3= P4 - P1 + 3.0f * (P2 - P3);
    
    return c0 + t*c1 + t*t*c2 + t*t*t*c3;
}
