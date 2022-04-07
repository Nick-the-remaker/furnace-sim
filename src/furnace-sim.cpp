#define CVUI_IMPLEMENTATION
#include "cvui.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

#define WINDOW_NAME "高炉模拟"

class simulator
{
public:
    simulator()
    {
        cv::FileStorage read("../config/param.yaml", cv::FileStorage::READ);
        read["steel"] >> steel;
        read["iron"] >> iron;
        read["z"] >> z;
        read["days"] >> days;
        read["count"] >> count;
        read.release();
        ofile.open("../report/report.md");
    }
    ~simulator() { ofile.close(); }
    void start()
    {
        cvui::init(WINDOW_NAME);
        while (true)
        {
            cv::Mat top(720, 1280, CV_8UC3, cv::Scalar(0, 0, 0));
            main_window = top.clone();
            cvui::window(main_window, 10, 10, 180, 700, "");
            cvui::text(main_window, 10, 20, "    I 0.95~1.10");
            cvui::trackbar(main_window, 10, 40, 120, &I, (float)0.95, (float)1.1, 1, "%.2Lf");

            cvui::text(main_window, 10, 100, "    i 1.0~1.25");
            cvui::trackbar(main_window, 10, 120, 120, &i, (float)1.0, (float)1.25, 1, "%.2Lf");

            cvui::text(main_window, 10, 180, "    N 8~12");
            cvui::trackbar(main_window, 10, 200, 120, &N, 8, 12);

            cvui::text(main_window, 10, 260, "    c 0.55~0.65");
            cvui::trackbar(main_window, 10, 280, 120, &c, (float)1.0, (float)1.25, 1, "%.2Lf");

            cvui::text(main_window, 10, 340, "    k 0.5~0.6");
            cvui::trackbar(main_window, 10, 360, 120, &k, (float)0.5, (float)0.6, 1, "%.2Lf");

            cvui::text(main_window, 10, 420, "    a 0.35~0.5");
            cvui::trackbar(main_window, 10, 440, 120, &a, (float)0.35, (float)0.5, 1, "%.2Lf");

            cvui::text(main_window, 10, 500, "    alpha 79~83");
            cvui::trackbar(main_window, 10, 520, 120, &alpha, (float)79, (float)83, 1, "%.2Lf");

            cvui::text(main_window, 10, 580, "    beta 81.5~85.5");
            cvui::trackbar(main_window, 10, 600, 120, &beta, (float)81.5, (float)85.5, 1, "%.2Lf");

            cvui::window(main_window, 140, 10, 150, 700, "");

            cvui::text(main_window, 140, 20, "    D/d 1.09~1.15");
            cvui::trackbar(main_window, 140, 40, 120, &D_sub_d, (float)1.09, (float)1.15, 1, "%.2Lf");

            cvui::text(main_window, 140, 100, "    d1/D 0.64~0.73");
            cvui::trackbar(main_window, 140, 120, 120, &d1_sub_D, (float)0.64, (float)0.73, 1, "%.2Lf");

            cvui::text(main_window, 140, 180, "    Hu/D 2.3~3.1");
            cvui::trackbar(main_window, 140, 200, 120, &Hu_sub_D, (float)2.3, (float)3.1, 1, "%.2Lf");

            cvui::update();

            draw_furnace();

            cv::imshow(WINDOW_NAME, main_window);
            char key = cv::waitKey(1);
            calculate(key);
            if (key == 's')
                cv::imwrite("../report/report.png", main_window);
            if (key == 27)
                break;
        }
    }

    // 计算参数
    void calculate(char output)
    {
        float ratio = 1 + z * (0.05);
        p = (iron * ratio + steel) * 10000 / days / count;
        volume = p / productivity;
        //计算炉缸直径
        d = 0.23 * sqrt(I * volume / i);
        float check1 = volume / (3.14 * d * d / 4);
        //计算渣口高度
        hz = 4 * p * 1.2 / (3.14 * d * d * N * c * 7.1);
        //计算风口高度
        hf = hz / k;
        //计算风口数目
        n = 2 * (d + 2);
        while ((int)n % 4 != 0)
        {
            (int)n++;
        }
        h1 = a + hf;
        //计算炉腰直径，炉腹角，炉腹高度
        D = d * D_sub_d;
        h2 = (D - d) / 2 * tan(alpha / 180 * 3.14);
        float check2 = atan(2 * h2 / (D - d)) * 180 / 3.14;
        //计算炉喉直径，炉喉高度
        d1 = d1_sub_D * D;
        //计算炉身角，炉身高度，炉腰高度
        h4 = tan(beta / 180 * 3.14) * (D - d1) / 2;
        float check3 = atan(2 * h4 / (D - d1)) * 180 / 3.14;
        Hu = Hu_sub_D * D;
        h3 = Hu - h1 - h2 - h4 - h5;
        //校核炉容
        float V1 = 3.14 / 4 * d * d * h1;
        float V2 = 3.14 / 12 * h2 * (D * D + D * d + d * d);
        float V3 = 3.14 / 4 * D * D * h2;
        float V4 = 3.14 / 12 * h4 * (D * D + D * d1 + d1 * d1);
        float V5 = 3.14 / 4 * d1 * d1 * h5;
        float Vu = V1 + V2 + V3 + V4 + V5;
        float deltaV = (Vu - volume) / volume;

        if (output == 'o')
        {
            std::cout << "日产量 = " << p << std::endl;
            std::cout << "容积 = " << volume << std::endl;
            std::cout << "炉缸直径 = " << d << std::endl;
            std::cout << "校核：Vu/A = " << check1;
            if (check1 < 28 && check1 > 22)
                std::cout << " 合理" << std::endl;
            else
                std::cout << " 不合理" << std::endl;
            std::cout << "渣口高度 = " << hz << std::endl;
            std::cout << "风口高度 = " << hf << std::endl;
            std::cout << "风口数目 = " << n << std::endl;
            std::cout << "风口结构尺寸 = " << a << std::endl;
            std::cout << "炉缸高度 = " << h1 << std::endl;
            std::cout << "死铁层厚度 = " << h0 << std::endl;
            std::cout << "炉腰直径 = " << D << std::endl;
            std::cout << "炉腹高度 = " << h2 << std::endl;
            std::cout << "校核: " << check2;
            if (check2 < 83 && check2 > 79)
                std::cout << " 合理" << std::endl;
            else
                std::cout << " 不合理" << std::endl;
            std::cout << "炉喉直径 = " << d1 << std::endl;
            std::cout << "炉身高度 = " << h4 << std::endl;
            std::cout << "校核: " << check3;
            if (check3 < 85.5 && check3 > 81.5)
                std::cout << " 合理" << std::endl;
            else
                std::cout << " 不合理" << std::endl;
            std::cout << "有效高度 = " << Hu << std::endl;
            std::cout << "炉腰高度 = " << h3 << std::endl;
            std::cout << V1 << " " << V2 << " " << V3 << " " << V4 << " " << V5 << " " << Vu << std::endl;
            std::cout << "deltaV = " << deltaV;
            if (deltaV < 0.5 && deltaV > 0)
                std::cout << " 炉型设计合理，符合要求" << std::endl;
            else
                std::cout << " 不合理" << std::endl;
        }
        if (output == 'r')
        {
            ofile << "> 此报告为开源软件**furnace-sim**自动生成，仅供学习和交流使用，请勿用于商业用途！" << std::endl;
            ofile << "> 如使用过程中发现软件错误，请联系 619597329@qq.com" << std::endl;
            ofile << "### 1.确定工作日: " << days << "d" << std::endl;
            ofile << "+ 日产量$P_总$ = " << (iron * ratio + steel) << "X 10000 / " << days << " = " << (iron * ratio + steel) * 10000 / days << "t" << std::endl;
            ofile << "### 2.确定容积: " << std::endl;
            ofile << "+ 选择设计高炉为" << count << "座，利用系数为" << productivity << "$t/m^3 \\cdot d$" << std::endl;
            ofile << "每座高炉日产量: $ P = P_总/" << count << " = " << p << "t$" << std::endl;
            ofile << "每座高炉容积: $ V_u' = P/ \\eta_v = " << p << "/" << count << " = " << volume << " m^3 $" << std::endl;
            ofile << "### 3.炉缸尺寸:" << std::endl;
            ofile << "+ 炉缸直径 " << std::endl;
            ofile << "选定冶炼强度I = " << I << "$t/m^3 \\cdot d$，燃烧强度$i_燃$"
                  << "=" << i << "$t/m^2 \\cdot h$" << std::endl;
            ofile << "计算d:" << std::endl;
            ofile << "d = " << d << "m" << std::endl;
            ofile << "校核 $V_u /A$:" << std::endl;
            ofile << "$V_u /A$ = " << check1 << "， 合理" << std::endl;
            ofile << "+ 炉缸高度: " << std::endl;
            ofile << "渣口高度$h_z$ :" << std::endl;
            ofile << "$h_z$ = " << hz << "m" << std::endl;
            ofile << "风口高度$h_f$: 取k=" << k << std::endl;
            ofile << "$h_f$ = " << hf << "m" << std::endl;
            ofile << "风口数目n: " << std::endl;
            ofile << "n = 2 (d+2) ，取 n= " << n << "个" << std::endl;
            ofile << "风口结构尺寸a:  取 a= " << a << "m" << std::endl;
            ofile << "则炉缸高度为 $h_1 = h_f + a = $" << h1 << "m" << std::endl;
            ofile << "+ 死铁层厚度: 选取$h_0$ = " << h0 << "m" << std::endl;
            ofile << "+ 炉腰直径，炉腹角，炉腹高度: " << std::endl;
            ofile << "取D/d = " << D_sub_d << std::endl;
            ofile << "则D/d = " << D_sub_d << "X" << d << " ，D = " << D << "m" << std::endl;
            ofile << "取 $\\alpha$ = " << alpha << "度，计算$h_2$" << std::endl;
            ofile << "$h_2 = $ " << h2 << "m" << std::endl;
            ofile << "校核 $\\alpha $" << std::endl;
            ofile << "$tg \\alpha = {2 h_2 \\over D-d}, "
                  << "\\alpha $= " << check2 << std::endl;
            ofile << "+  炉喉直径，炉喉高度: " << std::endl;
            ofile << "取$d_1 /D = $ " << d1_sub_D << std::endl;
            ofile << "则$d_1 = $ " << d1_sub_D << "X"
                  << "D = " << d1 << "m" << std::endl;
            ofile << "取$h_5$ =" << h5 << "m" << std::endl;
            ofile << "+ 炉身角，炉身高度，炉腰高度: " << std::endl;
            ofile << "取 $\\beta$ = " << beta << "度" << std::endl;
            ofile << "计算$h_4$: " << std::endl;
            ofile << "$h_4 = $ " << h4 << "m" << std::endl;
            ofile << "校核$ \\beta$ : " << std::endl;
            ofile << "$tg \\beta = {2 h_4 \\over D-d_1}, "
                  << "\\beta $= " << check3 << "度" << std::endl;
            ofile << "+ 取$H_u/D = $ " << Hu_sub_D << std::endl;
            ofile << "$H_u = $ " << Hu_sub_D << "X" << D << "，$H_u = $" << Hu << std::endl;
            ofile << "求得: $h_3 = H_u - h_1 - h_2 - h_4 - h_5 = $ " << h3 << "m" << std::endl;
            ofile << "+ 校核炉容: " << std::endl;
            ofile << "炉缸体积: $V_1 = $" << V1 << std::endl;
            ofile << "炉腹体积: $V_2 = $" << V2 << std::endl;
            ofile << "炉腰体积: $V_3 = $" << V3 << std::endl;
            ofile << "炉身体积: $V_4 = $" << V4 << std::endl;
            ofile << "炉喉体积: $V_5 = $" << V5 << std::endl;
            ofile << "高炉容积: $V_u = V_1 + V_2 + V_3 + V_4 + V_5 = $" << Vu << std::endl;
            ofile << "误差: " << std::endl;
            ofile << "$\\Delta$ V = " << deltaV << "%  < 0.5%" << std::endl;
            ofile << "炉型设计合理，符合要求 " << std::endl;
            cv::imwrite("../report/report.png", main_window);
            ofile << "+ 绘图 " << std::endl;
            ofile << "![](../report/report.png)" << std::endl;
        }
    }

    void draw_furnace()
    {
        // 底部中心点，所有以这一点为基准
        int x = 780;
        int y = 700;
        float scale = 15; //用于显示的放大倍数
        float d_ = 0, D_ = 0, d1_ = 0, h0_ = 0, h1_ = 0, h2_ = 0, h3_ = 0, h4_ = 0, h5_ = 0;
        d_ = d * scale;
        D_ = D * scale;
        d1_ = d1 * scale;
        h0_ = h0 * scale;
        h1_ = h1 * scale;
        h2_ = h2 * scale;
        h3_ = h3 * scale;
        h4_ = h4 * scale;
        h5_ = h5 * scale;
        cv::Point p2(x - d_ / 2 + h0_, y);
        cv::Point p3(x - d_ / 2, y - h0_);
        cv::Point p4(x - d_ / 2, y - h0_ - h1_);
        cv::Point p5(x - D_ / 2, y - h0_ - h1_ - h2_);
        cv::Point p6(x - D_ / 2, y - h0_ - h1_ - h2_ - h3_);
        cv::Point p7(x - d1_ / 2, y - h0_ - h1_ - h2_ - h3_ - h4_);
        cv::Point p8(x - d1_ / 2, y - h0_ - h1_ - h2_ - h3_ - h4_ - h5_);
        cv::Point p9(x + d1_ / 2, y - h0_ - h1_ - h2_ - h3_ - h4_ - h5_);
        cv::Point p10(x + d1_ / 2, y - h0_ - h1_ - h2_ - h3_ - h4_);
        cv::Point p11(x + D_ / 2, y - h0_ - h1_ - h2_ - h3_);
        cv::Point p12(x + D_ / 2, y - h0_ - h1_ - h2_);
        cv::Point p13(x + d_ / 2, y - h0_ - h1_);
        cv::Point p14(x + d_ / 2, y - h0_);
        cv::Point p15(x + d_ / 2 - h0_, y);

        cv::line(main_window, p2, p3, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p3, p4, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p4, p5, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p5, p6, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p6, p7, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p7, p8, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p8, p9, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p9, p10, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p10, p11, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p11, p12, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p12, p13, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p13, p14, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p14, p15, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p15, p2, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p3, p14, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p4, p13, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p5, p12, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p6, p11, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);
        cv::line(main_window, p7, p10, cv::Scalar(0, 255, 0), 4, cv::LINE_AA);

        cv::circle(main_window, p2, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p3, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p4, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p5, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p6, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p7, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p8, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p9, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p10, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p11, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p12, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p13, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p14, 4, cv::Scalar(255, 0, 0), -1);
        cv::circle(main_window, p15, 4, cv::Scalar(255, 0, 0), -1);
    }

public:
    cv::Mat main_window;
    float steel = 0, iron = 0, z = 0, days = 0, count = 0;
    float p = 0;            //日产量
    float productivity = 2; //高炉利用系数，默认2
    float volume = 0;       //一座高炉容积
    float d = 0;            //炉缸直径
    float hz = 0;           //渣口高度
    float hf = 0;           //风口高度
    int n = 0;              //风口数目
    float h1 = 0;           //炉缸高度
    float d1 = 0;           //炉喉直径
    float h2 = 0;           //炉腹高度
    float h3 = 0;           //炉腰高度
    float h4 = 0;           //炉身高度
    float h5 = 2;           //炉喉高度
    float h0 = 1.5;         //死铁层厚度
    float Hu = 0;           //有效高度
    float D = 0;            //炉腰直径

private:
    float I = 0.95;        //冶炼强度
    float i = 1.05;        //燃烧强度
    int N = 10;            //出铁次数
    float c = 0.6;         //渣口以下炉缸容积利用系数
    float k = 0.56;        //渣口高度与风口高度之比
    float a = 0.5;         //风口结构尺寸
    float alpha = 80.3;    //炉腹角
    float beta = 84;       //炉身角
    float D_sub_d = 1.13;  // D与d的比值
    float d1_sub_D = 0.68; // d1与D的比值
    float Hu_sub_D = 2.56; //有效高度与炉腰直径的比值
    std::ofstream ofile;   //输出报告
};

int main()
{
    simulator sim;
    sim.start();
    return 0;
}