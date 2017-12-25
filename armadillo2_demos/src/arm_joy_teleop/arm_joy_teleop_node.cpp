#include <ros/ros.h>
#include <sensor_msgs/Joy.h>

#include <control_msgs/GripperCommandAction.h>
#include <actionlib/client/simple_action_client.h>

#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>


#define DEBUG_JOY_ARM

typedef actionlib::SimpleActionClient<control_msgs::GripperCommandAction> GripperClient;


class ArmJoyNode {
private:
    ros::NodeHandle _nodeHandle;
    ros::AsyncSpinner _spinner;
    moveit::planning_interface::MoveGroupInterface _group;
    float _rotation1IncreamentValue;
    float _rotation2IncreamentValue;
    float _shoulder1IncreamentValue;
    float _shoulder2IncreamentValue;
    float _shoulder3IncreamentValue;
    float _wristIncreamentValue;
    // arm state
    int _rotation1State;
    int _rotation2State;
    int _shoulder1State;
    int _shoulder2State;
    int _shoulder3State;
    int _wristState;
    int _gripperState;

    //buttons and axis indexs
    int _deadManIndex;
    int _hardProfileIndex;
    int _softProfileIndex;
    int _xButtonIndex;
    int _yButtonIndex;
    int _aButtonIndex;
    int _bButtonIndex;
    int _upDownAxis;
    int _rightLeftAxis;
    bool _gripperWorking;

    ros::Subscriber _joySub;



    void joyCallback(const sensor_msgs::Joy::ConstPtr &msg) {
        bool isDeadManActive = msg->buttons[_deadManIndex] == 1;

        if(!isDeadManActive) {
            if(msg->axes[_hardProfileIndex] == -1.0) {

                if(msg->axes[_rightLeftAxis] == 1) {
                    _rotation1State = 1;
                } else if(msg->axes[_rightLeftAxis] == -1) {
                    _rotation1State = -1;
                }

                if(msg->axes[_upDownAxis] == 1) {
                    _shoulder1State = -1;
                } else if(msg->axes[_upDownAxis] == -1) {
                    _shoulder1State = 1;
                }

                if(msg->buttons[_xButtonIndex] == 1) {
                    _rotation2State = -1;
                } else if(msg->buttons[_bButtonIndex] == 1) {
                    _rotation2State = 1;
                }
                if(msg->buttons[_yButtonIndex] == 1) {
                    _shoulder2State = -1;
                } else if(msg->buttons[_aButtonIndex] == 1) {
                    _shoulder2State = 1;
                }


            } else if(msg->axes[_softProfileIndex] == -1.0) {
                if(msg->axes[_rightLeftAxis] == 1) {
                    _wristState = -1;
                } else if(msg->axes[_rightLeftAxis] == -1) {
                    _wristState = 1;
                }

                if(msg->axes[_upDownAxis] == 1) {
                    _shoulder3State = -1;
                } else if (msg->axes[_upDownAxis] == -1) {
                    _shoulder3State = 1;
                }

                if(msg->buttons[_yButtonIndex] == 1) {
                    _gripperState = 1;
                } else if(msg->buttons[_aButtonIndex] == 1) {
                    _gripperState = -1;
                }
            }
        }

    }
    bool haveMoveGoal() {
        return _rotation1State == 1 || _rotation1State == -1
               || _rotation2State == 1 || _rotation2State == -1
               || _shoulder1State == 1 || _shoulder1State == -1
               || _shoulder2State == 1 || _shoulder2State == -1
               || _shoulder3State == 1 || _shoulder3State == -1
               || _wristState == 1 || _wristState == -1
               || _gripperState == 1 || _gripperState == -1;
    }
public:
    ArmJoyNode(): _nodeHandle(),  _spinner(2), _group("arm")  {
        ROS_INFO("[%s]: Arm joy node is active", ros::this_node::getName().c_str());
        _rotation1State = 0;
        _rotation2State = 0;
        _shoulder1State = 0;
        _shoulder2State = 0;
        _shoulder3State = 0;
        _wristState = 0;
        _gripperState = 0;
        _gripperWorking = false;
        _joySub = _nodeHandle.subscribe<sensor_msgs::Joy>("joy", 10, &ArmJoyNode::joyCallback, this);
        _group.setPlannerId("RRTConnectkConfigDefault");
        _group.setPlanningTime(3.0);
        _group.setNumPlanningAttempts(10);



        ros::param::param<int>("drive_joy_teleop_deadman_button", _deadManIndex, 7);
        ros::param::param<int>("arm_joy_node_hard_profile_index", _hardProfileIndex, 4);
        ros::param::param<int>("arm_joy_node_soft_profile_index", _softProfileIndex, 5);
        ros::param::param<int>("arm_joy_node_x_button_index", _xButtonIndex, 3);
        ros::param::param<int>("arm_joy_node_y_button_index", _yButtonIndex, 4);
        ros::param::param<int>("arm_joy_node_a_button_index", _aButtonIndex, 0);
        ros::param::param<int>("arm_joy_node_b_button_index", _bButtonIndex, 1);
        ros::param::param<int>("arm_joy_node_up_down_index", _upDownAxis, 7);
        ros::param::param<int>("arm_joy_node_left_right_index", _rightLeftAxis, 6);

        ros::param::param<float>("arm_joy_node_rotation1", _rotation1IncreamentValue, 0.025);
        ros::param::param<float>("arm_joy_node_rotation2", _rotation2IncreamentValue, 0.1);
        ros::param::param<float>("arm_joy_node_shoulder1", _shoulder1IncreamentValue, 0.05);
        ros::param::param<float>("arm_joy_node_shoulder2", _shoulder2IncreamentValue, 0.05);
        ros::param::param<float>("arm_joy_node_shoulder3", _shoulder3IncreamentValue, 0.05);
        ros::param::param<float>("arm_joy_node_wrist", _wristIncreamentValue, 0.1);

        _spinner.start();
    }

    void run() {
        ros::Rate loopRate(40);

        ROS_INFO("Trying to move arm to 90 deg pos");
        _group.setNamedTarget("ninety_deg");
        moveit::planning_interface::MoveGroupInterface::Plan start_plan;
        if(_group.plan(start_plan))  //Check if plan is valid
        {
            ROS_INFO("valid pos");
            _group.execute(start_plan);
        }
        ROS_INFO("continueing...");


        std::vector<double> group_variable_values;
        group_variable_values.reserve(6);
        _group.getCurrentState()->copyJointGroupPositions(
                _group.getCurrentState()->getRobotModel()->getJointModelGroup(_group.getName()),
                group_variable_values);

        GripperClient ac("/gripper_controller/gripper_cmd", true);
        ROS_INFO("Waiting for action server to start.");
        // wait for the action server to start
        ac.waitForServer(); //will wait for infinite time

        ROS_INFO("Action server started.");
        control_msgs::GripperCommandGoal goal;
        while(ros::ok()) {
            if(haveMoveGoal()) {

                std::vector<double> prev_group_variable_values;
                prev_group_variable_values.reserve(6);

                if(_rotation1State == -1) {
                    _rotation1State = 0;
                    group_variable_values[0] -= _rotation1IncreamentValue;
                } else if(_rotation1State == 1) {
                    _rotation1State = 0;
                    group_variable_values[0] += _rotation1IncreamentValue;
                }

                if(_shoulder1State == -1) {
                    _shoulder1State = 0;
                    group_variable_values[1] -= _shoulder1IncreamentValue;
                } else if(_shoulder1State == 1) {
                    _shoulder1State = 0;
                    group_variable_values[1] += _shoulder1IncreamentValue;
                }

                if(_shoulder2State == -1) {
                    _shoulder2State = 0;
                    group_variable_values[2] -= _shoulder2IncreamentValue;
                } else if(_shoulder2State == 1) {
                    _shoulder2State = 0;
                    group_variable_values[2] += _shoulder2IncreamentValue;
                }

                if(_rotation2State == -1) {
                    _rotation2State = 0;
                    group_variable_values[3] -= _rotation2IncreamentValue;
                } else if(_rotation2State == 1) {
                    _rotation2State = 0;
                    group_variable_values[3] += _rotation2IncreamentValue;
                }

                if(_shoulder3State == -1) {
                    _shoulder3State = 0;
                    group_variable_values[4] -= _shoulder3IncreamentValue;
                } else if(_shoulder3State == 1) {
                    _shoulder3State = 0;
                    group_variable_values[4] += _shoulder3IncreamentValue;
                }

                if(_wristState == -1) {
                    _wristState = 0;
                    group_variable_values[5] -= _wristIncreamentValue;
                } else if(_wristState == 1) {
                    _wristState = 0;
                    group_variable_values[5] += _wristIncreamentValue;
                }

                if(_gripperState == -1) {
                    _gripperState = 0;
                    if(!_gripperWorking) {
                        // send a goal to the action


                        goal.command.position=0.001;
                        goal.command.max_effort=5;
                        ac.sendGoal(goal);

                    } else {
                        ROS_WARN("[%s]: Gripper already have a goal please wait", ros::this_node::getName().c_str());
                    }

                } else if(_gripperState == 1) {
                    _gripperState = 0;
                    if (!_gripperWorking) {
                        goal.command.position=0.1;
                        goal.command.max_effort=5;
                        ac.sendGoal(goal);

                    } else {
                        ROS_WARN("[%s]: Gripper already have a goal please wait", ros::this_node::getName().c_str());
                    }

                }

                for (int i=0; i<6; i++)
                    prev_group_variable_values[i] = group_variable_values[i];


                _group.setJointValueTarget(group_variable_values);
                moveit::planning_interface::MoveGroupInterface::Plan my_plan;
                bool success = (_group.plan(my_plan)==moveit::planning_interface::MoveItErrorCode::SUCCESS);
                if (success) {
                    _group.move();
                }
                else {
                    ROS_WARN("[%s]: Invalid goal", ros::this_node::getName().c_str());
                    //if fail, revert to prev values
                    for (int i=0; i<6; i++)
                        group_variable_values[i] = prev_group_variable_values[i];
                }
            }

            loopRate.sleep();
        }
    }



};

int main(int argc, char** argv) {

    ros::init(argc, argv, "arm_joy_node");
    ArmJoyNode armJoy;
    armJoy.run();

    return 0;

}
