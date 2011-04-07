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

#include "AnimationProcessor.h"

#include "Baker.h"

#include "Utils.h"

#include "common_const.h"

#include <boost/scoped_array.hpp>

#include "ColladaBakeryConfig.h"

using namespace ColladaBakery;

AnimationProcessor::AnimationProcessor(Baker* baker) :
    Processor(baker), _output_is_transposed(false)
{}

bool AnimationProcessor::process(const CF::Object* c_obj) {

    const CF::Animation* c_anim = static_cast<const CF::Animation*>(c_obj);
    
    if (c_anim->getAnimationType() == CF::Animation::ANIMATION_CURVE) {

        const CF::AnimationCurve* c_anim_curve = 
                                static_cast<const CF::AnimationCurve*>(c_anim);

        _c_id = c_anim_curve->getUniqueId();
        //The following is just a string representation for the COLLADA id
        string c_id = _baker->get_id(c_anim_curve);
        _rtr_anim.set_id(c_id);
        //Create the sampler for TIME input

        //assert that all physical input dimensions are TIME
        if ( c_anim_curve->getInPhysicalDimension() != 
              CF::PHYSICAL_DIMENSION_TIME )
        {
            cout << "Warning:" << endl;
            cout << "We are only supporting input physical dimension of TIME."
                 << "Cannot import animation " << c_id << " which has another "
                 << "input dimension." << endl;
            //this is not critical, just skip import
            return true;
        }

        //we don't really care about the output physical dimension, we assume
        //that the mapping within the COLLADA document makes sense (e.g. 
        //PHYSICAL_DIMENSION "angle" animations the angle input of a rotation
        //transform, etc...

        rtr_format::Animation_Sampler* rtr_time_s = _rtr_anim.add_sampler();
        _rtr_input_sampler = rtr_time_s;
        
        //Note that we can have only one sampler per animation the way we import
        //it at the moment. This might have to change if we decided to group
        //animations.
        rtr_time_s->set_id(c_id + "_input_sampler");

        //Notice this note from the COLLADA 1.4 SPEC: By convention, the 
        //interpolation method for a segment is attached to the first point.

        //Also note that COLLADA 1.4 supports only cubic bezier splines.

        //Time sampler always has only one component
        rtr_time_s->set_components(1);

        if (c_anim_curve->getKeyCount() == 0) {
            cout << "Key count zero. Skipping animation." <<endl;
            return true;
        }

        //We have N-1 segments where N is number of keyframes.
        size_t num_segments = c_anim_curve->getKeyCount() - 1;
        //rtr_time_s->set_segment_count(num_segments);

        //TODO: emulate linear interpolation, convert HERMITE

        CF::AnimationCurve::InterpolationType c_global_int_type = 
                                          c_anim_curve->getInterpolationType();

        //create the data sampler
        rtr_format::Animation_Sampler* rtr_data_s = _rtr_anim.add_sampler();
        _rtr_output_sampler = rtr_data_s;

        rtr_data_s->set_id(c_id + "_data_sampler");

        size_t c_out_dimension = c_anim_curve->getOutDimension();

        rtr_data_s->set_components(c_out_dimension);
        //rtr_data_s->set_segment_count(num_segments);

        bool has_tangents = (c_anim_curve->getInterpolationType() == 
                             CF::AnimationCurve::INTERPOLATION_BEZIER) || 
                            (c_anim_curve->getInterpolationType() == 
                             CF::AnimationCurve::INTERPOLATION_HERMITE) ||
                            (c_anim_curve->getInterpolationType() == 
                             CF::AnimationCurve::INTERPOLATION_MIXED);

        //asert types
        assert( c_anim_curve->getInputValues().getType() == 
                CF::FloatOrDoubleArray::DATA_TYPE_FLOAT );

        assert( c_anim_curve->getOutputValues().getType() == 
                CF::FloatOrDoubleArray::DATA_TYPE_FLOAT );

        //If not tangent values are used, the data type would be 
        //DATA_TYPE_UNKNOWN, therefore we assert this only when we are
        //dealing with HERMITE or BEZIER interpolation types.
        if ( has_tangents ) {
            assert( c_anim_curve->getInTangentValues().getType() == 
                    CF::FloatOrDoubleArray::DATA_TYPE_FLOAT );
            assert( c_anim_curve->getOutTangentValues().getType() == 
                    CF::FloatOrDoubleArray::DATA_TYPE_FLOAT );
        }

        const float* c_in_values = 
                 c_anim_curve->getInputValues().getFloatValues()->getData();

        const float* c_out_values = 
                 c_anim_curve->getOutputValues().getFloatValues()->getData();

        const float* c_in_tangents = NULL;

        const float* c_out_tangents = NULL;

        if (has_tangents) {
            c_in_tangents = 
                c_anim_curve->getInTangentValues().getFloatValues()->getData();
            c_out_tangents = 
               c_anim_curve->getOutTangentValues().getFloatValues()->getData();
        }

        //per-keyframe stride for accessing in and out tangents
        size_t stride = 1 + c_out_dimension;

        //Note: Obviously, when exporting mixed interpolation types, the 
        //tangent arrays are still filled properly, even if not really required
        //for vertain interpolation types (e.g. LINEAR, or STEP)

        //Unfortunately, the OpenCOLLADA API does not tell use how to access
        //the tangent streams explicitly, intuitively (as suggested by the
        //spec) we would expect [KEY1, DIM1, DIM2, DIM3]*, however
        //we get [KEY1, DIM1, KEY1, DIM2, KEY1, DIM3]
        //For now we just assume the latter and double-check this case.
        //It would be nice to have a safer way of importing this in future.

        //There is another caveat when loading animated colors. Obviously, 
        //OpenCOLLADA exports animated colors always as a 4-component animation
        //Unfortunately, the tangent array looks then like this: 
        //[KEY1, R, KEY1, G, KEY1, B, 0, 0] --> notice the padded 0, 0 pair.
        //We have to take this into consideration when checking our overly
        //complex "workaround"

        //Asserting the above assumptions
        bool oc_tangent_check = true;
       
        if (has_tangents && (c_out_dimension > 1) ) {

            size_t val_c = c_anim_curve->getInTangentValues().getValuesCount();
            //tangent value count must be exactly twice the amount
            //of (num_segments + 1) times out dimension as we have one key
            //per out dimension value in OpenCOLLADA
            oc_tangent_check &= (val_c == (num_segments+1)*2*c_out_dimension);
            val_c = c_anim_curve->getOutTangentValues().getValuesCount();
            oc_tangent_check &= (val_c == (num_segments+1)*2*c_out_dimension);

            //We also assume that the repeat in-key for the tangents is always
            //the same in each key-frame. If this would not be the case, we 
            //could not import this as one animation as we support only one 
            //timesampler per channel
            //Note: also see the color special case description above
            for ( size_t iKey = 0;
                  iKey < num_segments;
                  ++iKey )
            {
                //first value, subsequent keys must be the same
                float check_value_in = c_in_tangents[iKey*c_out_dimension*2];
                float check_value_out = c_out_tangents[iKey*c_out_dimension*2];
                for (size_t iDim = 1; iDim < c_out_dimension; ++iDim) {

                    int idx = iKey*c_out_dimension*2 + iDim*2;

                    bool validity_check = false;

                    //Special case for colors where the padded 4th component
                    //in-out pair is always 0 0
                    if ( (c_out_dimension == 4) && 
                         (iDim == 3) ) 
                    {
                        validity_check = true;
                    } else {
                        validity_check = 
                            (c_in_tangents[idx] == check_value_in) &&
                            (c_out_tangents[idx] == check_value_out);
                    }

                    oc_tangent_check &= validity_check;
                }
            }
            
        }

        //TODO: if importing from another package supporting more complex, 
        //we should catch the "normal" case in here as well.
        if (!oc_tangent_check) {
            cout << "Check for OpenCOLLADA exported multi-dim output "
                 << " assumptions failed." << endl;
            cout << "Animation will not be imported." << endl;
            return false;
        } else {
            //fix the stride as we have to access tangent values differently
            //now that we know how they are stored
            stride = c_out_dimension*2;
        }

        //We assemble time and data control points
        //
        //We will structure them like this:
        //
        //Time Control points: 
        //  [t0, t1, t2, t3,...] where each segment re-uses the last element
        //as the first for the next segment:
        //  [t0, t1, t2, t3], [t3, t4, t5, t6],...
        //
        //Data Control points (e.g. for 3 dimension data):
        //  [d0x,d1x,d2x,d3x,d0y,d1y,d2y,d3y,d0z,d1z,d2z,d3z,...]
        //again the last element of a segment is re-used for the next segment
        //Note that we store the data values interleaved for a more efficient
        //memory layout.

        //We need a temporary vector to assemble output data
        //At a later point we will convert them to interleaved data
        vector<float> data_s_tmp;

        vector<CF::AnimationCurve::InterpolationType> interpolation_types;

        interpolation_types.resize(c_anim_curve->getKeyCount(), c_global_int_type);

        //prefill array
        if (c_global_int_type == CF::AnimationCurve::INTERPOLATION_MIXED) {
            for (size_t iKey = 0; iKey < c_anim_curve->getKeyCount(); ++iKey) {
                interpolation_types[iKey] = c_anim_curve->getInterpolationTypes()[iKey];
            }
        }

        insert_step_interpolation(c_anim_curve, interpolation_types);

        //iterate over keyframes and set accordingly
        for ( size_t iKey = 0;
              iKey < num_segments;
              ++iKey )
        {

            CF::AnimationCurve::InterpolationType c_int_type = interpolation_types.at(iKey);

            if (!( (c_int_type == CF::AnimationCurve::INTERPOLATION_BEZIER) ||
                   (c_int_type == CF::AnimationCurve::INTERPOLATION_LINEAR) ||
                   (c_int_type == CF::AnimationCurve::INTERPOLATION_STEP)) )
            {
                cout << "Unsupported interpolation type. We support BEZIER, "
                     << " LINEAR and STEP only." << endl;
                return false;
            }
            
            //Note that the first control point of a segment is shared with
            //with the last control point of the previous segment (implicitly)
            //Except for the first, naturally.

            //For convenience we directly store the addresses of begin 
            //and end values
            const float * c_in_begin = &c_in_values[iKey];
            const float * c_in_end = &c_in_values[iKey+1];
            const float * c_out_begin = &c_out_values[iKey*c_out_dimension];
            const float * c_out_end = &c_out_values[(iKey+1)*c_out_dimension];

            //Control point 0 (only written for the very first control point)
            //All other starting control points re-use the previous last
            //control point of a segment
            if (iKey == 0) {
                rtr_time_s->add_control_point(*c_in_begin);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back( c_out_begin[iC] );
                }
            }

            if (c_int_type == CF::AnimationCurve::INTERPOLATION_BEZIER) 
            {
                //For bezier interpolation we will use the tangent arrays 
                //that define the middle-control points
                
                //Control point 1 (stored in out-tangent array)
                rtr_time_s->add_control_point(c_out_tangents[iKey*stride]);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    int idx = iKey*stride+iC+1;
                    if (oc_tangent_check) {
                        //OpenCOLLADA fix
                        idx = iKey*stride+iC*2 + 1;
                    }
                    data_s_tmp.push_back( c_out_tangents[idx] );
                }            
            
                //Control point 2 (stored in in-tangent array)
                rtr_time_s->add_control_point(c_in_tangents[(iKey+1)*stride]);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    int idx = (iKey+1)*stride+iC+1;
                    if (oc_tangent_check) {
                        //OpenCOLLADA fix
                        idx = (iKey+1)*stride+iC*2 + 1;
                    }
                    data_s_tmp.push_back( c_in_tangents[idx] );
                }                        
            } 
            else if (c_int_type == CF::AnimationCurve::INTERPOLATION_LINEAR)
            {
                float a = (1.0f-1.0f/3.0f);
                float b = 1.0f/3.0f;

                //Control point 1
                float in_cp1 = a*(*c_in_begin) + b*(*c_in_end);
                rtr_time_s->add_control_point(in_cp1);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    float out_cp1 = a*(c_out_begin[iC]) + b*(c_out_end[iC]);
                    data_s_tmp.push_back( out_cp1);
                }

                a = (1.0f-2.0f/3.0f);
                b = 2.0f/3.0f;

                //Control point 2
                float in_cp2 = a*(*c_in_begin) + b*(*c_in_end);
                rtr_time_s->add_control_point(in_cp2);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    float out_cp2 = a*(c_out_begin[iC]) + b*(c_out_end[iC]);
                    data_s_tmp.push_back(out_cp2);
                }

            } 
            else if (c_int_type == CF::AnimationCurve::INTERPOLATION_STEP)
            {
                //To simulate a step function with bezier interpolation, we
                //need to add an add an additional in-between control point

                float a = (1.0f-1.0f/3.0f);
                float b = 1.0f/3.0f;

                //Control point 1
                float in_cp1 = a*(*c_in_begin) + b*(*c_in_end);
                rtr_time_s->add_control_point(in_cp1);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back( c_out_begin[iC] );
                }

                a = (1.0f-2.0f/3.0f);
                b = 2.0f/3.0f;

                //Control point 2
                float in_cp2 = a*(*c_in_begin) + b*(*c_in_end);
                rtr_time_s->add_control_point(in_cp2);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back(c_out_begin[iC]);
                }

                //Control point 3
                rtr_time_s->add_control_point(*c_in_end);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back(c_out_begin[iC]);
                }

                rtr_time_s->add_control_point(*c_in_end);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back(c_out_end[iC]);
                }

                rtr_time_s->add_control_point(*c_in_end);
                for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                    data_s_tmp.push_back(c_out_end[iC]);
                }

            } 
            else 
            {
                cout << "Unknown interpolation type" << endl;
                return false;
            }

            //Control point 3 (this is also the next starting control point)
            rtr_time_s->add_control_point(c_in_values[iKey+1]);
            for (size_t iC = 0; iC < c_out_dimension; ++iC) {
                int target_idx = iC*(num_segments*3+1) + iKey*3+3;
                data_s_tmp.push_back( c_out_end[iC] );
            }

        }

        //we will now insert the data from the temp array in an interleaved
        //fashion. This should be more cache-efficient when evaluating
        //bezier data directly
        
        //determine the actual size of the written keyframes. Due to STEP
        //interpolation might have had to add intermediate keyframes.

        size_t out_num_segments = (rtr_time_s->control_point_size() - 1) / 3;

        for (size_t iC = 0; iC<c_out_dimension; ++iC) {
            for (size_t iKey = 0; iKey < out_num_segments; iKey++) {

                //CP0
                if (iKey == 0) {
                    rtr_data_s->add_control_point(data_s_tmp.at(iC));
                }

                //CP1
                int idx = (iKey * 3 + 1) * c_out_dimension + iC;
                rtr_data_s->add_control_point(data_s_tmp.at(idx));

                //CP2
                idx = (iKey * 3 + 2) * c_out_dimension + iC;
                rtr_data_s->add_control_point(data_s_tmp.at(idx));

                //CP3
                idx = (iKey * 3 + 3) * c_out_dimension + iC;
                rtr_data_s->add_control_point(data_s_tmp.at(idx));

            }
        }

        rtr_time_s->set_segment_count(out_num_segments);
        rtr_data_s->set_segment_count(out_num_segments);

    } else {
        cout << "Warning: Animation of type Formula is not supported." << endl;
        return true;
    }

    //as we successfully processed the animation, let's register for
    //postprocess which creates the channel entries and writes out 
    //into protobuffer format
    _baker->register_for_postprocess(shared_from_this(), 1);

    return true;
}

bool AnimationProcessor::post_process() {

    //ask for animation bindings which contains this animations UNIQUE ID, 
    //then map. ONLY those animation bindings that we actually registered.

    //Then create the proper time/data sampler to target mapping.

    size_t num_cnls = _baker->cache().animation_resolved_bindings.count(_c_id);

    BakerCache::AnimToRTRBindingMap::const_iterator it = 
                        _baker->cache().animation_resolved_bindings.find(_c_id);

    for (;num_cnls > 0; ++it,num_cnls--) {
    //while (num_cnls > 0) {

        const BakerCache::RTRBinding& rtr_binding = it->second;

        //resolve targets
        string rtr_target_string = rtr_binding.rtr_id;
        const CF::AnimationList::AnimationBinding& c_ab = 
                                                     rtr_binding.c_anim_binding;

        //We don't map everything the same way as COLLADA does. If we address
        //somehting that we have split into its own fields (e.g. angle in
        //rotation transform element, we use a "/" to address
        //e.g. -> myRotate/angle
        //If we were to index somehting, that we also have as a separate 
        //component (e.g. translation vector), we can use "." for addressing
        //e.g. -> myTranslation.X
        //Note that for translation or scale, we don't have subcomponents to
        //address

        //TODO: complete with more adapters to addressing syntaxes
        if (c_ab.animationClass == CF::AnimationList::POSITION_XYZ) {

            //this requires us to have a 3 component output dimension
            if (_rtr_output_sampler->components() != 3) {
                cout << "Error: Cannot add channel with POSITION_XYZ addressing"
                     << " since output sampler does not have DIM=3." << endl;
                continue;
            }

            //no need to add anything to address sub-components or sub-values

        } else if (c_ab.animationClass == CF::AnimationList::MATRIX4X4) {

            //this requires us to have a 16 component output dimension
            if (_rtr_output_sampler->components() != 16) {
                cout << "Error: Cannot add channel with MATRIX4x4 addressing"
                     << " since output sampler does not have DIM=16." << endl;
                continue;
            }

            //We know that COLLADA store row-major matrices, therefore we need
            //to transpose the component data of the output data (once)
            if (!_output_is_transposed)
                transpose_mat_output();

            //no need to add anything to address sub-components or sub-values
        } else if (c_ab.animationClass == CF::AnimationList::FLOAT) {

            //this requires us to have a 1 component output dimension
            if (_rtr_output_sampler->components() != 1) {
                cout << "Error: Cannot add channel with FLOAT addressing"
                     << " since output sampler does not have DIM=1." << endl;
                continue;
            }

            //no need to add anything to address sub-components or sub-values
        } else if (c_ab.animationClass == CF::AnimationList::POSITION_X) {
            
            if (_rtr_output_sampler->components() != 1) {
                cout << "Error: Cannot add channel with POSITION_X addressing"
                     << " since output sampler does not have DIM=1." << endl;
                continue;
            }

            rtr_target_string += ".X";

        } else if (c_ab.animationClass == CF::AnimationList::POSITION_Y) {
            
            if (_rtr_output_sampler->components() != 1) {
                cout << "Error: Cannot add channel with POSITION_Y addressing"
                     << " since output sampler does not have DIM=1." << endl;
                continue;
            }

            rtr_target_string += ".Y";

        } else if (c_ab.animationClass == CF::AnimationList::POSITION_Z) {
            
            if (_rtr_output_sampler->components() != 1) {
                cout << "Error: Cannot add channel with POSITION_Z addressing"
                     << " since output sampler does not have DIM=1." << endl;
                continue;
            }

            rtr_target_string += ".Z";

        } else if (c_ab.animationClass == CF::AnimationList::ANGLE) {
            
            if (_rtr_output_sampler->components() != 1) {
                cout << "Error: Cannot add channel with ANGLE addressing"
                     << " since output sampler does not have DIM=1." << endl;
                continue;
            }

            rtr_target_string += rtr::Targets::ROTATE_ANGLE_TARGET();
        
        //Workaround for OpenCOLLADA where a 4-component coloro animation is
        //not recognized as animation class RGBA
        } else if ( (c_ab.animationClass == CF::AnimationList::UNKNOWN_CLASS) &&
                    (_rtr_output_sampler->components() == 4) )
        {
            cout << "Warning: Animation " << _rtr_anim.id() << " with an "
                 << " unspecified type of addressing and components of 4 " 
                 << " is assumed to be a RGBA color animation."
                 << endl;

            //no further sub-addressing necessary
        } else {
            cout << "Warning: Animation " << _rtr_anim.id() << " uses an "
                 << " unsupported type of addressing syntax." 
                 << " Skipping import." << endl;
            continue;
        }

        //create a new animation channel for this binding
        rtr_format::Animation_Channel * rtr_cnl = _rtr_anim.add_channel();
        
        //we also need to register the ID of the rtr-animation with 
        //the binding it was created from. This is going to be used by the
        //visual scene postprocessor stage
        _baker->cache().animation_binding_results[rtr_binding.c_anim_list_id].push_back(_rtr_anim.id());

        //TODO: the way we import right now, we won't need multiple channels
        //per animation, there would also be one data and one time sampler.
        //Maybe we can reformat that

        rtr_cnl->set_time_sampler(_rtr_input_sampler->id());
        rtr_cnl->set_data_sampler(_rtr_output_sampler->id());

        rtr_cnl->set_target(rtr_target_string);

        //Also print out log info about hard cut insertions of this animation
        if (!_hard_cut_insertions.empty()) {
            cout << "Animation Target '" << rtr_target_string << "' has hard"
                 << " cut insertions: @<time> with <extracted_diff>: " << endl;

            HardCutLogInfoList::const_iterator it;
            for (it = _hard_cut_insertions.begin(); 
                 it != _hard_cut_insertions.end();
                 ++it)
            {
                cout << "    @ " << it->time << " with " << it->diff << endl;
            }
        }
    }

    //finally bake out the animation
    bool b = _baker->write_baked(_rtr_anim.id(), &_rtr_anim);
    if (!b)
        return false;

    return true;
}

void AnimationProcessor::transpose_mat_output() {

    int num_segments = _rtr_output_sampler->segment_count();

    if (_rtr_output_sampler->components() != 16) {
        cout << "Error: Cannot transpose a sampler with number of components "
             << " != 16." << endl;
        return;
    }

    //Transpose
    for (int iRow = 0; iRow < 4; ++iRow) {
        for (int iCol = 0; iCol < 4; ++iCol) {

            //We do not need to transpose the diagonal, and we only need two 
            //visit the lower half of the matrix
            if ( iRow <= iCol )
                continue;

            int src_mat_idx = iRow * 4 + iCol;
            int dst_mat_idx = iCol * 4 + iRow;

            for ( int iEl = 0; 
                  iEl < num_segments*3+1;
                  ++iEl )
            {
                int src = src_mat_idx * (num_segments * 3 + 1) + iEl;
                int dst = dst_mat_idx * (num_segments * 3 + 1) + iEl;
                _rtr_output_sampler->mutable_control_point()->SwapElements(src, 
                                                                           dst);
            }

        }
    }

    _output_is_transposed = !_output_is_transposed;
}

void AnimationProcessor::insert_step_interpolation(const CF::AnimationCurve * c_anim_curve,
                                                   vector<CF::AnimationCurve::InterpolationType>& interpolation_types)
{
    size_t c_out_dimension = c_anim_curve->getOutDimension();
    size_t num_segments =  c_anim_curve->getKeyCount() - 1;

    const float* c_in_values = 
        c_anim_curve->getInputValues().getFloatValues()->getData();

    const float* c_out_values = 
        c_anim_curve->getOutputValues().getFloatValues()->getData();

    double average_diff = 0;

    for ( size_t iSegment = 0;
          iSegment < num_segments;
          ++iSegment )
    {

        average_diff += calc_diff(c_in_values, 
                                  c_out_values, 
                                  iSegment, 
                                  c_out_dimension);

    }

    average_diff /= num_segments;

    for ( size_t iSegment = 0;
          iSegment < num_segments;
          ++iSegment )
    {

        double diff   = calc_diff(c_in_values, 
                                  c_out_values, 
                                  iSegment, 
                                  c_out_dimension);

        if (diff > average_diff * bakery_config.step_threshold()) {
            interpolation_types[iSegment] = CF::AnimationCurve::INTERPOLATION_STEP;
            HardCutLogInfo info;
            info.time = c_in_values[iSegment];
            info.diff = diff;
            _hard_cut_insertions.push_back(info);
        }

    }
}

float AnimationProcessor::calc_diff(const float* c_in_arr, const float* c_out_arr, int num_segment, size_t c_out_dimension)
{
    const float * c_in = &c_in_arr[num_segment];
    const float * c_out = &c_out_arr[num_segment*c_out_dimension];

    const float * c_in_next = &c_in_arr[num_segment+1];
    const float * c_out_next = &c_out_arr[(num_segment+1)*c_out_dimension];

    double dt = *c_in_next - *c_in;

    if (dt == 0)
        return 0;

    double diff = 0;
    for (size_t iC = 0; iC < c_out_dimension; ++iC) {
        float tmp = (c_out_next[iC] - c_out[iC]);
        diff += tmp*tmp;
    }

    return sqrt(diff)/dt;
}

