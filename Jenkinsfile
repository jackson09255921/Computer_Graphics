pipeline {
    agent any

    parameters {
        booleanParam(name: 'RUN_GPU_TESTS', defaultValue: false,
            description: 'Run the optional CUDA test on a Windows NVIDIA GPU agent')
    }

    options {
        timestamps()
        timeout(time: 30, unit: 'MINUTES')
        disableConcurrentBuilds()
    }

    stages {
        stage('Configure') {
            steps {
                script {
                    if (isUnix()) {
                        sh 'cmake -S . -B build-jenkins -DBUILD_LEGACY_LABS=OFF -DCMAKE_BUILD_TYPE=Release'
                    } else {
                        bat 'cmake -S . -B build-jenkins -A x64 -DBUILD_LEGACY_LABS=ON'
                    }
                }
            }
        }

        stage('Build and test') {
            steps {
                script {
                    if (isUnix()) {
                        sh 'cmake --build build-jenkins --parallel'
                        sh 'ctest --test-dir build-jenkins --output-on-failure'
                    } else {
                        bat 'cmake --build build-jenkins --config Release --parallel'
                        bat 'ctest --test-dir build-jenkins -C Release --output-on-failure'
                    }
                }
            }
        }

        stage('Render smoke tests') {
            steps {
                script {
                    if (isUnix()) {
                        sh './build-jenkins/bezier_demo bezier_demo.bmp'
                        sh './build-jenkins/raytracer_demo raytracer_demo.bmp'
                        sh './build-jenkins/pathtracer_demo pathtracer_demo.bmp 8'
                    } else {
                        bat 'build-jenkins\\Release\\bezier_demo.exe bezier_demo.bmp'
                        bat 'build-jenkins\\Release\\raytracer_demo.exe raytracer_demo.bmp'
                        bat 'build-jenkins\\Release\\pathtracer_demo.exe pathtracer_demo.bmp 8'
                    }
                }
            }
        }

        stage('CUDA capability') {
            when {
                expression { return params.RUN_GPU_TESTS }
            }
            steps {
                script {
                    if (isUnix()) {
                        error('RUN_GPU_TESTS currently expects the configured Windows RTX 4060 agent')
                    }
                    bat 'nvidia-smi'
                    bat 'cmake -S . -B build-jenkins-cuda -A x64 -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=ON -DCG_CUDA_ARCHITECTURES=89'
                    bat 'cmake --build build-jenkins-cuda --config Release --target cuda_capability'
                    bat 'ctest --test-dir build-jenkins-cuda -C Release -R cuda_capability --output-on-failure'
                }
            }
        }
    }

    post {
        always {
            archiveArtifacts artifacts: '*.bmp', allowEmptyArchive: true
            junit testResults: 'build-jenkins/**/Test.xml', allowEmptyResults: true
        }
    }
}
