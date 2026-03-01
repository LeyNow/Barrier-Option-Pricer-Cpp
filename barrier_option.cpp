#include "barrier_option_h.hpp"

thread_local double sumPayoff_thread = 0.0;
thread_local double option_price_thread = 0.0;
thread_local double prix_sj_local = 0.0;
thread_local int state = 0;

class BarrierPricer {

    private :

    std::string m_type, m_inout, m_updown; // Call or Put / Knock-in or Knock-out / Up or Down
    unsigned int M, N, T; // number of path / number of observation's date / maturity
    double sigma, K, R, S0, m_barrier_value; // volatility / strike / risk-free interest rate / spot / barrier's price
    const int number_thread = 5;
    double option_price = 0.0;
    std::vector<double> option_price_vector;
    std::mutex mtx;

    public :

    // Builder
    BarrierPricer(const std::string type, const std::string inout, const std::string updown, const unsigned int nombre_trajectoire, const unsigned int date_observation, const double vol, const double prix_sj_init, const double barrier_value, const double strike, const double taux_r, const unsigned int maturite)
    : m_type(type), m_inout(inout), m_updown(updown), M(nombre_trajectoire), N(date_observation), sigma(vol), S0(prix_sj_init), m_barrier_value(barrier_value), K(strike), R(taux_r), T(maturite) {}


    double générateur_loi_normale() { 
        static thread_local std::mt19937 gen(std::random_device{}());
        std::normal_distribution<double> dist(0.0, 1.0);
        return dist(gen);
    }

    void calculate_option_price_barrier() {
        double dt = T / static_cast<double>(N);
        prix_sj_local = S0;
        state = 0;
        sumPayoff_thread = 0.0;

        for (int i = 0; i < M / number_thread; i++) {
            prix_sj_local = S0;
            for (int j = 0; j < N; j++) {
                double z = générateur_loi_normale();
                prix_sj_local *= exp((R - 0.5 * pow(sigma, 2)) * dt + sigma * sqrt(dt) * z);

                if (m_updown == "up") {
                    if (prix_sj_local >= m_barrier_value) {
                        state = 1;
                    }
                }
                else { // down
                    if (prix_sj_local <= m_barrier_value) {
                        state = 1;
                    }
                }
                
            }

            if (m_inout == "in") { 
                if (state == 1) { // la barrière a été passée
                    if (m_type == "call") { 
                        sumPayoff_thread += std::max(prix_sj_local - K, 0.0);
                    }
                    else { // put
                        sumPayoff_thread += std::max(K - prix_sj_local, 0.0);
                    }
                }
            }
            else { // knock-out  
                if (state == 0) { // la barrière n'a pas été passée
                    if (m_type == "call") { 
                        sumPayoff_thread += std::max(prix_sj_local - K, 0.0);
                    }
                    else { // put
                        sumPayoff_thread += std::max(K - prix_sj_local, 0.0);
                    }
                }
            }
            state = 0;
        }
        option_price_thread = sumPayoff_thread / static_cast<double>(M / number_thread);
        option_price_thread *= exp(-R * T);
        option_price_vector.push_back(option_price_thread);
    }

    double calculate_final_price() {
        std::vector<std::thread> threads;
        double sumPayoff_total = 0.0;

        std::thread t1(&BarrierPricer::calculate_option_price_barrier, this);
        std::thread t2(&BarrierPricer::calculate_option_price_barrier, this);
        std::thread t3(&BarrierPricer::calculate_option_price_barrier, this);
        std::thread t4(&BarrierPricer::calculate_option_price_barrier, this);
        std::thread t5(&BarrierPricer::calculate_option_price_barrier, this);
        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();

        for (int i = 0; i < number_thread; i++) {

            sumPayoff_total += option_price_vector[i];
        }

        option_price = sumPayoff_total / static_cast<double>(number_thread);
        return option_price;
    }


    void user_finish() {
        std::cout << "\n------------- End Barrier Option Pricer -------------" << std::endl;
        std::cout << "Estimation de la valeur de l'option : " << option_price;
    }
};


int main() {
    std::string option_type, in_or_out_, updown_ = "";
    double vol_, S0_, K_, R_, barrier_ = 0.0;
    unsigned int M_, N_, T_ = 0;

    std::cout << "------------- Start Barrier Option Pricer -------------" << std::endl;
    std::cout << "Type d'option (call ou put) : ";
    std::cin >> option_type;
    std::cout << "\nKnock-in ou Knock-out ? : ";
    std::cin >> in_or_out_;
    std::cout << "\nUp ou Down : ";
    std::cin >>  updown_;
    std::cout << "\nNombre de trajectoire : ";
    std::cin >> M_;
    std::cout << "\nNombre de dates d'observations : ";
    std::cin >> N_;
    std::cout << "\nVolatilite : ";
    std::cin >> vol_;
    std::cout << "\nPrix du sous-jacent a t = 0 : ";
    std::cin >> S0_;
    std::cout << "\nBarriere : ";
    std::cin >> barrier_;
    std::cout << "\nStrike K : ";
    std::cin >> K_;
    std::cout << "\nTaux R : ";
    std::cin >> R_;
    std::cout << "\nMaturite T :  ";
    std::cin >> T_;

    auto start = std::chrono::high_resolution_clock::now(); // début du chrono
    BarrierPricer barrier(option_type, in_or_out_, updown_, M_, N_, vol_, S0_, barrier_, K_, R_, T_);
    barrier.calculate_final_price();
    auto end = std::chrono::high_resolution_clock::now(); // fin du chrono
    barrier.user_finish();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nTemps de calcul : " << duration.count() << " ms";
    return 0;
}