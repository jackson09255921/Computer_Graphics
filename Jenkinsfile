pipeline {
    agent any

    parameters {
        booleanParam(name: 'RUN_GPU_TESTS', defaultValue: false,
            description: 'Run the optional CUDA test on a Windows NVIDIA GPU agent')
        booleanParam(name: 'RUN_QUALITY_RENDERS', defaultValue: false,
            description: 'Publish slower 512 spp CPU and 128 spp CUDA gallery images (requires RUN_GPU_TESTS)')
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

        stage('Build CPU') {
            steps {
                script {
                    if (isUnix()) {
                        sh 'cmake --build build-jenkins --parallel'
                    } else {
                        bat 'cmake --build build-jenkins --config Release --parallel'
                    }
                }
            }
        }

        stage('Build CUDA') {
            when {
                expression { return params.RUN_GPU_TESTS }
            }
            steps {
                script {
                    if (isUnix()) {
                        sh '/home/cgdev/miniconda3/bin/conda run -n computer-graphics cmake -S . -B build-jenkins-cuda -G Ninja -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=ON -DCG_CUDA_ARCHITECTURES=89 -DCMAKE_BUILD_TYPE=Release'
                        sh '/home/cgdev/miniconda3/bin/conda run -n computer-graphics cmake --build build-jenkins-cuda --parallel'
                    } else {
                        bat 'nvidia-smi'
                        bat 'cmake -S . -B build-jenkins-cuda -A x64 -DBUILD_LEGACY_LABS=OFF -DBUILD_CUDA_DEMOS=ON -DCG_CUDA_ARCHITECTURES=89'
                        bat 'cmake --build build-jenkins-cuda --config Release --parallel'
                    }
                }
            }
        }

        stage('Validation pipeline') {
            steps {
                script {
                    if (isUnix()) {
                        if (params.RUN_GPU_TESTS) {
                            sh 'python3 tools/run_validation_pipeline.py --build-dir build-jenkins --gpu-build-dir build-jenkins-cuda'
                        } else {
                            sh 'python3 tools/run_validation_pipeline.py --build-dir build-jenkins'
                        }
                    } else {
                        if (params.RUN_GPU_TESTS) {
                            bat 'python tools\\run_validation_pipeline.py --build-dir build-jenkins --gpu-build-dir build-jenkins-cuda'
                        } else {
                            bat 'python tools\\run_validation_pipeline.py --build-dir build-jenkins'
                        }
                    }
                }
            }
        }

        stage('Quality renders') {
            when {
                expression { return params.RUN_GPU_TESTS && params.RUN_QUALITY_RENDERS }
            }
            steps {
                script {
                    if (isUnix()) {
                        sh 'python3 tools/run_quality_renders.py --cpu-build-dir build-jenkins --gpu-build-dir build-jenkins-cuda'
                    } else {
                        bat 'python tools\\run_quality_renders.py --cpu-build-dir build-jenkins --gpu-build-dir build-jenkins-cuda'
                    }
                }
            }
        }
    }

    post {
        always {
            archiveArtifacts artifacts: 'build-jenkins/validation-artifacts/**/*,build-jenkins/*visual*.ppm,build-jenkins/*diff*.ppm,LabX/images/**/*.png', allowEmptyArchive: true
            junit testResults: 'build-jenkins/validation-artifacts/validation-report.xml', allowEmptyResults: true
        }
    }
}
