//
// Created by lenovo on 2020/2/23.
//

#ifndef ANN_EA_ROBOT_EA_H
#define ANN_EA_ROBOT_EA_H

#include "./ANN.h"
#include "./Robot.h"
#include "./Map.h"
#include  <omp.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>

template <class T = GridMap>
class ANNRobot
{
public:
    Robot robot = Robot({200,200},0.0);
    Ann controller = Ann(12,2,4);
    T map;
    double fitness = 0.0;
public:
    ANNRobot();
    ANNRobot(const ANNRobot & C);
    void RunOneStep();
    void Fitness();
};

template <class T = GridMap>
class EA
{
public:
    vector<ANNRobot<T>> robot_group;
    vector<double> fitness_group;
    int generation = 100;
    int population = 20;
private:
    int max_envolution_steps = 300;
    int nth_max_selection_rank = 4;

public:
    void EAInitialization();
    void Selection();
    void FitnessAll();
    void Crossover();
    void PopulationUpdate();
    void Mutation();
    void Envolution();
    void ClearGeneration();
    double GetAveFitness();
    double GetBestFitness();
    ANNRobot<T> GetBestControl(bool verbose = false);
};

static default_random_engine e((unsigned)time(0));
static uniform_real_distribution<double> n_uniform_x(50,350);
static uniform_real_distribution<double> n_uniform_y(50,150);
static uniform_real_distribution<double> n_uniform_ang(-PI,PI);

void weight_swap(double* a1, double* a2,int length ,int pos)
{
    double* tmp = new double[length];
    for(int i = 0; i < length - pos; i++)
    {
        tmp[i + pos] = a1[i + pos];
    }
    for(int i = 0; i < length - pos; i++)
    {
        a1[i + pos] = a2[i + pos];
    }
    for(int i = 0; i < length - pos; i++)
    {
        a2[i + pos] = tmp[i + pos];
    }
    delete tmp;
}

template<class T> ANNRobot<T>::ANNRobot()
{
    //random distribution
    //this->robot = Robot({n_uniform_x(e),n_uniform_y(e)},n_uniform_ang(e));
    this->robot = Robot({200,200},0.0);
}

template<class T> ANNRobot<T>::ANNRobot(const ANNRobot & C)
{
    //random distribution
//    this->robot = Robot({n_uniform_x(e),n_uniform_y(e)},n_uniform_ang(e));
    this->robot = Robot({200,200},0.0);
    this->controller = Ann(C.controller);
}

template<class T> void ANNRobot<T>::RunOneStep()
{
    this->robot.ClearData();
    this->robot.GetAllData(this->map.wall_set);

//    cout<<"sensors_data\t";
//    for(int i = 0; i < this->robot.sensors_data.size(); i++)
//    {
//        cout<<this->robot.sensors_data[i]<<"\t";
//    }
//    cout<<endl;

    this->controller.EvolveOneStep(this->robot.sensors_data);

//    cout<<"hidden\t";
//    for(int i = 0; i < this->controller.HiddenLayer.size(); i++)
//    {
//        cout<<this->controller.HiddenLayer[i].value<<"\t";
//    }
//    cout<<endl;

//    cout<<"output\t";
//    for(int i = 0; i < this->controller.OutputLayer.size(); i++)
//    {
//        cout<<this->controller.OutputLayer[i].value<<"\t";
//    }
//    cout<<endl;

    this->robot.l_speed = this->controller.OutputLayer[0].value;
    this->robot.l_speed = max(-this->robot.v_bound,min(this->robot.v_bound,this->robot.l_speed));
    this->robot.r_speed = this->controller.OutputLayer[1].value;
    this->robot.r_speed = max(-this->robot.v_bound,min(this->robot.v_bound,this->robot.r_speed));

//    this->robot.l_speed = this->robot.v_bound;
//    this->robot.r_speed = this->robot.v_bound;

//    cout<<"before move\t";
//    cout <<"\tpos:\t"<< this->robot.center_pose.x()
//         <<'\t'<<this->robot.center_pose.y()
//         <<"\tspeed:\t"<<this->robot.l_speed
//         <<'\t'<<this->robot.r_speed<<endl;
    this->robot.Move(this->map.virtual_wall_set);
//    cout<<"after move\t";
//    cout <<"\tpos:\t"<< this->robot.center_pose.x()
//         <<'\t'<<this->robot.center_pose.y()
//         <<"\tspeed:\t"<<this->robot.l_speed
//         <<'\t'<<this->robot.r_speed<<endl;
    this->map.grid_map.block(min(359,max(0,(int)this->robot.center_pose.y() - 20)),
                             min(359,max(0,(int)this->robot.center_pose.x() - 20)),40,40).setOnes();
}
template<class T> void  ANNRobot<T>::Fitness()
{
    this->fitness = max(0.0,this->map.grid_map.sum()  - this->robot.collision_times * 1000 - this->robot.diff_l_r * 10);
}

template<class T> void EA<T>::EAInitialization()
{
    for(int i = 0; i < this->population; i++)
    {
        ANNRobot<T> individual = ANNRobot<T>();
        this->robot_group.push_back(individual);
        //cout << "robot ； " << i << "\tSecond Matrix : \n" <<  individual.controller.SecondWeightMatrix<<endl;
    }
}

template<class T> void EA<T>::FitnessAll()
{
    this->fitness_group.resize(this->robot_group.size());
//# pragma omp parallel for
    for(int i = 0; i < this->robot_group.size(); i++)
    {
        this->robot_group[i].Fitness();
        this->fitness_group[i] = this->robot_group[i].fitness;
//        cout << "robot : " << i << "\tfitness : " << this->robot_group[i].fitness
//        << "\tcollision times : " << this->robot_group[i].robot.collision_times
//        << "\taverage speed : " << (this->robot_group[i].robot.l_speed + this->robot_group[i].robot.r_speed) / 2 << endl;
    }
}

template<class T> double EA<T>::GetAveFitness()
{
    double sum = 0.0;
    for(int i = 0; i < this->fitness_group.size();i++)
    {
        sum += this->fitness_group[i];
    }
    return sum/this->fitness_group.size();
}

template<class T> double EA<T>::GetBestFitness()
{
    double best_fitness = 0.0;
    for(int i = 0; i < this->fitness_group.size();i++)
    {
        best_fitness = max(this->fitness_group[i],best_fitness);
    }
    return best_fitness;
}

template<class T> void EA<T>::Envolution()
{
//# pragma omp parallel for
    for(int i = 0; i < this->robot_group.size(); i++)
    {
        for(int j = 0; j < this->max_envolution_steps;j++)
        {
            this->robot_group[i].RunOneStep();
//            cout << "robot : " << i << "\tsteps : " << j
//                 << "\tl_speed : " <<  this->robot_group[i].robot.l_speed << "\tr_speed : " <<  this->robot_group[i].robot.r_speed
//                 << "\tdirection : " <<  this->robot_group[i].robot.direction << "\n" << this->robot_group[i].robot.center_pose << endl;

//            cout << "robot id:\t"<<i<<"\tsteps:\t"<<j<<"\tpos:\t"<< this->robot_group[i].robot.center_pose.x()
//                 <<'\t'<<this->robot_group[i].robot.center_pose.y()
//                 <<"\tspeed:\t"<<this->robot_group[i].robot.l_speed
//                 <<'\t'<<this->robot_group[i].robot.r_speed<<endl;
        }
        //cout << "robot : " << i << "\tgrid map" << this->robot_group[i].map.grid_map.sum() << endl;
    }
}

template<class T> void EA<T>::Selection()
{
//    cout<<"\nbefore this->fitness_group:\t";
//    for(int i = 0; i < this->fitness_group.size();i++)
//    {
//        cout<<this->fitness_group[i]<<'\t';
//    }
//    cout<<endl;

//    nth_element(this->fitness_group.begin(), this->fitness_group.begin() + this->nth_max_selection_rank,
//                this->fitness_group.end(), std::greater<double>());
    sort(this->fitness_group.rbegin(),this->fitness_group.rend());
    this->fitness_group.erase(unique(this->fitness_group.begin(), this->fitness_group.end()), this->fitness_group.end());

//    cout<<"after this->fitness_group:\t";
//    for(int i = 0; i < this->fitness_group.size();i++)
//    {
//        cout<<this->fitness_group[i]<<'\t';
//    }
//    cout<<endl;

    double nth_max_selection_rank_fitness = this->fitness_group[this->nth_max_selection_rank];
//    cout<<"nth_max_selection_rank_fitness\t"<<nth_max_selection_rank_fitness<<endl;
    vector<ANNRobot<T>> robot_group_tmp;

    for(int j = 0; j < nth_max_selection_rank;j++)
    {
        for(int i = 0; i < this->robot_group.size(); i++)
        {
            if(this->robot_group[i].fitness == this->fitness_group[j])
            {
                robot_group_tmp.push_back(ANNRobot<T>(this->robot_group[i]));
//                cout<<"select robot : " << i<<"\tselect fitness:"<<this->robot_group[i].fitness<<endl;
                break;


//            cout << "select robot : " << i << "\tSecondWeightMatrix : \n" << this->robot_group[i].controller.SecondWeightMatrix <<endl;

            }
        }
    }

//    cout<<"robot_group_tmp.size():\t"<<robot_group_tmp.size()<<endl;
    this->ClearGeneration();
    this->robot_group = robot_group_tmp;
    robot_group_tmp.clear();

//    for(int i = 0; i < this->robot_group.size(); i++)
//    {
//        cout << "robot : " << i << "\tSecondWeightMatrix : \n" << this->robot_group[i].controller.SecondWeightMatrix <<endl;
//    }
//    for(int i = 0; i < this->robot_group.size(); i++)
//    {
//        cout << "robot : " << i << "\tFirstWeightMatrix : \n" << this->robot_group[i].controller.FirstWeightMatrix <<endl;
//    }
}

template<class T> void EA<T>::ClearGeneration()
{
    this->fitness_group.clear();
    this->robot_group.clear();
}

template<class T> void EA<T>::Crossover()
{
//# pragma omp parallel for
    for(int i = 0; i < (this->population - this->nth_max_selection_rank) / 2; i++)
    {
        uniform_int_distribution<int> n_uniform_int(0,this->nth_max_selection_rank - 1);
        int parent1,parent2;

        do{
            parent1 = n_uniform_int(e);
            parent2 = n_uniform_int(e);
        }while(parent1 == parent2);

        int first_weight_size = this->robot_group[parent1].controller.FirstWeightMatrix.size();
        int second_weight_size = this->robot_group[parent1].controller.SecondWeightMatrix.size();
        int weight_size = this->robot_group[parent2].controller.FirstWeightMatrix.size() + this->robot_group[parent2].controller.SecondWeightMatrix.size();

        double* p1_firstweight = this->robot_group[parent1].controller.FirstWeightMatrix.data();
        double* p1_secondweight = this->robot_group[parent1].controller.SecondWeightMatrix.data();
        double* p1_weight = new double[weight_size];
        for(int i = 0; i < first_weight_size;i++)
        {
            p1_weight[i] = p1_firstweight[i];
        }
        for(int i = 0; i < second_weight_size;i++)
        {
            p1_weight[i + first_weight_size] = p1_secondweight[i];
        }
        double* p2_firstweight = this->robot_group[parent2].controller.FirstWeightMatrix.data();
        double* p2_secondweight = this->robot_group[parent2].controller.SecondWeightMatrix.data();
        double* p2_weight = new double[weight_size];
        for(int i = 0; i < first_weight_size;i++)
        {
            p2_weight[i] = p2_firstweight[i];
        }
        for(int i = 0; i < second_weight_size;i++)
        {
            p2_weight[i + first_weight_size] = p2_secondweight[i];
        }

        uniform_real_distribution<double> n_uniform_method(0,1);
        double method = n_uniform_method(e);
        if(method < 0.05)
        {
            //cout<<"method1"<<endl;
            uniform_int_distribution<int> n_uniform_int(0,weight_size);
            int pos = n_uniform_int(e);
            weight_swap(p1_weight,p2_weight,weight_size,pos);
        }
        else if(method < 0.55)
        {
            //cout<<"method2"<<endl;
            uniform_int_distribution<int> n_uniform_int2(0,20);
            int j_size = n_uniform_int(e);
            for(int j = 0; j < j_size; j++)
            {
                uniform_int_distribution<int> n_uniform_int(0,weight_size);
                int pos = n_uniform_int(e);
                weight_swap(p1_weight,p2_weight,weight_size,pos);
            }
        }
        else
        {
            //cout<<"method3"<<endl;
            for(int i = 0; i < weight_size; i++)
            {
                p1_weight[i] = (2 * p1_weight[i] + p2_weight[i]) / 3.0;
                p2_weight[i] = (p2_weight[i] + p1_weight[i]) / 2.0;
            }
        }

        ANNRobot<T> child1;
        child1.controller.FirstWeightMatrix = Map<MatrixXd>(p1_weight,child1.controller.InputLayer.size(),child1.controller.HiddenLayer.size()- 1);
        child1.controller.SecondWeightMatrix = Map<MatrixXd>(&p1_weight[first_weight_size],child1.controller.HiddenLayer.size(),child1.controller.OutputLayer.size());

        ANNRobot<T> child2;
        child2.controller.FirstWeightMatrix = Map<MatrixXd>(p2_weight,child2.controller.InputLayer.size(),child2.controller.HiddenLayer.size()- 1);
        child2.controller.SecondWeightMatrix = Map<MatrixXd>(&p2_weight[first_weight_size],child2.controller.HiddenLayer.size(),child2.controller.OutputLayer.size());

        this->robot_group.push_back(child1);
        this->robot_group.push_back(child2);

        delete[] p1_weight;
        delete[] p2_weight;

//        if((child1.controller.SecondWeightMatrix!=this->robot_group[parent1].controller.SecondWeightMatrix
//            ||child1.controller.SecondWeightMatrix!=this->robot_group[parent2].controller.SecondWeightMatrix)
//           && (method < 0.55))
//        {
//            cout<<"HAHAHAHAHAHAHAHAHA"<<endl;
//            cout << "parent1 : SecondWeightMatrix : \n" << this->robot_group[parent1].controller.SecondWeightMatrix <<endl;
//            cout << "parent2 : SecondWeightMatrix : \n" << this->robot_group[parent2].controller.SecondWeightMatrix <<endl;
//            cout << "child1 : SecondWeightMatrix : \n" << child1.controller.SecondWeightMatrix <<endl;
//            cout << "child2 : SecondWeightMatrix : \n" << child2.controller.SecondWeightMatrix <<endl;
//        }
    }

//    for(int i = 0; i < this->robot_group.size(); i++)
//    {
//        cout << "robot : " << i << "\tSecondWeightMatrix : \n" << this->robot_group[i].controller.SecondWeightMatrix <<endl;
//    }
}

template<class T> void EA<T>::PopulationUpdate()
{
    this->population = this->robot_group.size();
}

template<class T> void EA<T>::Mutation()
{
    this->PopulationUpdate();
    //cout<<"population : "<<population<< "\t size : "<< this->robot_group.size()<<endl;
//# pragma omp parallel for
    for(int i = 0; i < population; i++)
    {
        uniform_real_distribution<double> n_uniform_prob(0,1);
        double mutaion_prob = n_uniform_prob(e);
        if(mutaion_prob < 0.8)
        {
            for(int j = 0; j < this->robot_group[i].controller.InputLayer.size(); j++)
            {
                for(int k = 0; k < this->robot_group[i].controller.HiddenLayer.size() - 1 ;k++)
                {
                    double unit_mutation_prob = n_uniform_prob(e);
//                    if(unit_mutation_prob > 0.95)
//                    {
//                        cout<<"before:\t"<<j<<"\t"<<k<<"\t"<<this->robot_group[i].controller.FirstWeightMatrix(j,k)<<endl;
//                    }
                    if(unit_mutation_prob > 0.975)
                    {
                        this->robot_group[i].controller.FirstWeightMatrix(j,k) += 0.1;
                    }
                    else if(unit_mutation_prob > 0.95)
                    {
                        this->robot_group[i].controller.FirstWeightMatrix(j,k) -= 0.1;
                    }
//                    if(unit_mutation_prob > 0.95)
//                    {
//                        cout<<"after:\t"<<j<<"\t"<<k<<"\t"<<this->robot_group[i].controller.FirstWeightMatrix(j,k)<<endl;
//                    }
                }
            }

            for(int j = 0; j < this->robot_group[i].controller.HiddenLayer.size(); j++)
            {
                for(int k = 0; k < this->robot_group[i].controller.OutputLayer.size() ;k++)
                {
                    double unit_mutation_prob = n_uniform_prob(e);
                    if(unit_mutation_prob > 0.95)
                    {
                        this->robot_group[i].controller.SecondWeightMatrix(j,k) += 0.5;
                    }
                    else if(unit_mutation_prob > 0.90)
                    {
                        this->robot_group[i].controller.SecondWeightMatrix(j,k) -= 0.5;
                    }
                }
            }
        }
    }
}

template<class T> ANNRobot<T> EA<T>::GetBestControl(bool verbose)
{
    int best_id = 0;
    double best_fitness = 0.0;
    for(int i = 0; i < this->population; i++)
    {
        if(this->robot_group[i].fitness > best_fitness)
        {
            best_fitness = this->robot_group[i].fitness;
            best_id = i;
        }
    }
    cout<<"best id : " << best_id << "\tfitness : " << best_fitness <<"\tcollision times : "<<this->robot_group[best_id].robot.collision_times<<endl;
    if(verbose)
    {
        Mat img = Mat::zeros(Size(400, 400), CV_8UC3);
        img.setTo(255);
        this->robot_group[best_id].map.map_show(img);
        Point last_pose = Point(200,200);
        for(auto pos : this->robot_group[best_id].robot.path)
        {
//            cout<<"path\t"<<pos.x()<<'\t'<<pos.y()<<endl;
            Point current_pose = Point(pos.x(),pos.y());
            line(img, last_pose, current_pose, Scalar(0, 0, 0), 3);
            last_pose = current_pose;
        }

        imshow("robot path", img);
        waitKey(0);
    }

    return this->robot_group[best_id];
}

#endif //ANN_EA_ROBOT_EA_H
