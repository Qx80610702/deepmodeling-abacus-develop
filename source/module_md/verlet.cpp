#include "verlet.h"

#include "md_func.h"
#include "module_base/timer.h"

Verlet::Verlet(MD_parameters& MD_para_in, UnitCell& unit_in) : MD_base(MD_para_in, unit_in)
{
}

Verlet::~Verlet()
{
}

void Verlet::setup(ModuleESolver::ESolver* p_esolver, const int& my_rank, const std::string& global_readin_dir)
{
    ModuleBase::TITLE("Verlet", "setup");
    ModuleBase::timer::tick("Verlet", "setup");

    MD_base::setup(p_esolver, my_rank, global_readin_dir);

    ModuleBase::timer::tick("Verlet", "setup");
}

void Verlet::first_half(const int& my_rank, std::ofstream& ofs)
{
    ModuleBase::TITLE("Verlet", "first_half");
    ModuleBase::timer::tick("Verlet", "first_half");

    MD_base::update_vel(force, my_rank);
    MD_base::update_pos(my_rank);

    ModuleBase::timer::tick("Verlet", "first_half");
}

void Verlet::second_half(const int& my_rank)
{
    ModuleBase::TITLE("Verlet", "second_half");
    ModuleBase::timer::tick("Verlet", "second_half");

    MD_base::update_vel(force, my_rank);
    apply_thermostat(my_rank);

    ModuleBase::timer::tick("Verlet", "second_half");
}

void Verlet::apply_thermostat(const int& my_rank)
{
    double t_target = 0;
    t_current = MD_func::current_temp(kinetic, ucell.nat, frozen_freedom_, allmass, vel);

    if (mdp.md_type == "nve")
    {
    }
    else if (mdp.md_thermostat == "rescaling")
    {
        t_target = MD_func::target_temp(step_ + step_rst_, mdp.md_nstep, mdp.md_tfirst, mdp.md_tlast);
        if (abs(t_target - t_current) * ModuleBase::Hartree_to_K > mdp.md_tolerance)
        {
            thermalize(0, t_current, t_target);
        }
    }
    else if (mdp.md_thermostat == "rescale_v")
    {
        if ((step_ + step_rst_) % mdp.md_nraise == 0)
        {
            t_target = MD_func::target_temp(step_ + step_rst_, mdp.md_nstep, mdp.md_tfirst, mdp.md_tlast);
            thermalize(0, t_current, t_target);
        }
    }
    else if (mdp.md_thermostat == "anderson")
    {
        if (my_rank == 0)
        {
            double deviation;
            for (int i = 0; i < ucell.nat; ++i)
            {
                if (static_cast<double>(std::rand()) / RAND_MAX <= 1.0 / mdp.md_nraise)
                {
                    deviation = sqrt(mdp.md_tlast / allmass[i]);
                    for (int k = 0; k < 3; ++k)
                    {
                        if (ionmbl[i][k])
                        {
                            vel[i][k] = deviation * MD_func::gaussrand();
                        }
                    }
                }
            }
        }
#ifdef __MPI
        MPI_Bcast(vel, ucell.nat * 3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#endif
    }
    else if (mdp.md_thermostat == "berendsen")
    {
        t_target = MD_func::target_temp(step_ + step_rst_, mdp.md_nstep, mdp.md_tfirst, mdp.md_tlast);
        thermalize(mdp.md_nraise, t_current, t_target);
    }
    else
    {
        ModuleBase::WARNING_QUIT("Verlet", "No such thermostat!");
    }
}

void Verlet::thermalize(const int& nraise, const double& current_temp, const double& target_temp)
{
    double fac = 0;
    if (nraise > 0 && current_temp > 0 && target_temp > 0)
    {
        fac = sqrt(1 + (target_temp / current_temp - 1) / nraise);
    }
    else if (nraise == 0 && current_temp > 0 && target_temp > 0)
    {
        fac = sqrt(target_temp / current_temp);
    }

    for (int i = 0; i < ucell.nat; ++i)
    {
        vel[i] *= fac;
    }
}

void Verlet::outputMD(std::ofstream& ofs, const bool& cal_stress, const int& my_rank)
{
    MD_base::outputMD(ofs, cal_stress, my_rank);
}

void Verlet::write_restart(const int& my_rank, const std::string& global_out_dir)
{
    MD_base::write_restart(my_rank, global_out_dir);
}

void Verlet::restart(const int& my_rank, const std::string& global_readin_dir)
{
    MD_base::restart(my_rank, global_readin_dir);
}