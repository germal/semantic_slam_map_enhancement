//
// Created by han on 18/2/21.
//

#include <map_localiser/CoordinatesMatcherNode.h>

int main(int argc, char **argv) {
    ros::init(argc, argv, "matcher_node");
    ros::NodeHandle nh;
    CoordinatesMatcherNode node(nh);
    ros::spin();
}

CoordinatesMatcherNode::CoordinatesMatcherNode(const ros::NodeHandle &nh) {
    ROS_INFO("initialising matcher node");

    //load floorplan
    ROS_INFO("loading floorplan");
    loadFloorplan();
    ROS_INFO("floorplan loaded");

    //matcher parameters
    int top, strategy;
    double min_correlation_threshold;
    int min_chain_length, max_buffer_size;
    nh_.getParam("matcher/top", top);
    nh_.getParam("matcher/strategy", strategy);
    nh_.getParam("matcher/min_correlation_threshold", min_correlation_threshold);
    nh_.getParam("matcher/min_chain_length", min_chain_length);
    nh_.getParam("matcher/max_buffer_size", max_buffer_size);
    matcher_ = new CoordinatesMatcher(top, min_correlation_threshold, min_chain_length, max_buffer_size, strategy);

    nh_.getParam("matcher/landmarks_topic", landmarks_topic_);
    nh_.getParam("matcher/match_result_topic", match_result_topic_);
    sub_ = nh_.subscribe(landmarks_topic_, 10, &CoordinatesMatcherNode::matchCallback, this);
    pub_ = nh_.advertise<map_localiser::MatchResult>(match_result_topic_, 10);

    ROS_INFO("matcher node ready");
}

void CoordinatesMatcherNode::loadFloorplan() {
    std::string floorplan_file_path;
    bool aggregate;
    nh_.getParam("matcher/floorplan_file_path", floorplan_file_path);
    nh_.getParam("matcher/floorplan_landmark_aggregation", aggregate);
    fp_.loadFromFile(floorplan_file_path, aggregate);

    if (fp_.hasError()) {
        ROS_ERROR("failed to load floor plan: %s", fp_.getError().c_str());
    }
}

void CoordinatesMatcherNode::matchCallback(map_localiser::ExtractorLandmarks landmarks) {
    map_localiser::MatchResult result = matcher_->match(landmarks, fp_);

    //summary ros info
    std::ostringstream summary;
    summary << "Found " << result.chains.size() << " chains" << std::endl;
    summary << "pattern_len=" << result.pattern.size() << std::endl;
    for (const auto &chain : result.chains) {
        summary << "corr=" << chain.correlation << std::endl;
    }
    ROS_INFO("%s", summary.str().c_str());

    result.header.stamp = ros::Time::now();
    pub_.publish(result);
}

CoordinatesMatcherNode::~CoordinatesMatcherNode() {
    delete matcher_;
}
