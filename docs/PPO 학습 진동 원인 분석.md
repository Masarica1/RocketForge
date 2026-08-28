# CleanRL 학습 진동 분석 및 CleanRL-2 테스트 보고서

- 작성일: 2026-08-28 (Asia/Seoul)
- 기준 Run: [CleanRL-1 (`i27qa9xw`)](https://wandb.ai/leeh2-yonsei/rocketforge/runs/i27qa9xw)
- 테스트 Run: [CleanRL-2 (`49qod1v9`)](https://wandb.ai/leeh2-yonsei/rocketforge/runs/49qod1v9)
- 테스트 태그: `Codex`
- 테스트 상태: `finished`

## 1. 결론 요약

CleanRL-1의 큰 진동은 **자연스러운 학습 분산 및 현재 로깅 방식이 만든 지연/계단 효과**와 **실제 후기 정책 성능 회귀**가 섞인 현상이다. 따라서 W&B의 `episode_reward`, `episode_length` 곡선 모양만 보고 전부 알고리즘 문제로 판단하면 안 되지만, 완전히 무시할 수 있는 현상도 아니다.

이번 CleanRL-2는 후기 정책 드리프트를 줄이기 위해 학습률 감소, entropy 보너스 완화, KL 제한을 함께 적용했다. 결과는 **부분 성공**이다.

- 마지막 25% 구간 `step_reward_mean` 평균은 사실상 동일했고(0.21817 → 0.21829), 표준편차는 14.1% 감소했다.
- 50-update 이동평균의 최고점 대비 최종 유지율은 80.0% → 98.5%로 좋아졌다.
- 후기 `episode_length`의 step-weighted 표준편차는 29.4% 감소했고 post-peak 최대 하락률도 45.9% → 41.1%로 완화됐다.
- 반면 최고 `episode_length`는 10,800 → 10,080(-6.7%), 최종값은 9,295 → 7,441(-19.9%)로 낮아졌다.
- 새 설정에서도 episode 단위 진동은 사라지지 않았으며, 중반 수렴은 더 느렸다.

따라서 **현재 CleanRL-2의 세 변경을 그대로 기본값으로 채택하는 것은 권장하지 않는다.** 후기 rollout 안정성 개선 신호는 유효하지만, 한 번의 seed와 세 변수를 동시에 바꾼 테스트이므로 다음에는 `anneal_lr: true`만 단독 검증하는 것이 우선이다.

## 2. 분석 범위와 방법

W&B API에서 두 Run의 config, summary, 상태, 태그 및 업로드 파일을 확인했다. 수치 비교는 W&B UI smoothing 값이 아니라 로컬에 동기화된 두 `.wandb` 원시 history 전체를 같은 방식으로 읽어 계산했다.

- 두 Run 모두 effective timestep: 1,998,848
- PPO update 수: 각각 976
- 기록된 history row: 각각 499,712
- CleanRL-1의 `episode_length` 실제 값 변경 횟수: 671
- CleanRL-1의 `episode_reward` 실제 값 변경 횟수: 732
- 환경, seed, 네트워크, 총 timestep, rollout/batch 구조는 동일하게 유지

`episode_*`에는 동일한 30-episode 이동평균이 매 환경 step마다 반복 기록된다. 따라서 다음 두 기준을 함께 사용했다.

1. W&B에서 보이는 시간축 특성을 반영한 step-weighted 통계
2. 즉시성이 더 높은 PPO rollout 단위 `step_reward_mean` 통계

## 3. CleanRL-1 진동 원인 분석

### 3.1 표시되는 큰 진동의 일부는 자연스럽거나 측정 방식에서 발생한다

1. **학습 정책 자체가 stochastic이다.** Actor는 3개의 Bernoulli action을 sampling한다. `randomize_initial_state: true`이고 초기 상태도 매 episode 달라지므로 동일한 정책에서도 episode 결과가 크게 달라질 수 있다.

2. **`episode_length`와 `episode_reward`는 독립적인 증거가 아니다.** 생존 중 매 step 양의 기본 보상이 있고 종료 시 `-10`을 받는다. CleanRL-1에서 두 지표의 상관계수는 0.9980이었다. 즉 두 곡선이 함께 진동하는 것은 두 개의 별도 실패 신호라기보다 거의 같은 생존 시간 신호를 두 번 본 것이다.

3. **30-episode 이동평균의 시간 폭이 학습 중 계속 변한다.** 초기에 episode 길이가 약 500이면 30개 창은 약 15,000 global timestep을 나타내지만, 상한 10,800에서는 약 324,000 timestep, 즉 전체 학습의 약 16.2%를 포함한다. 후기 곡선은 현재 정책보다 오래된 정책 결과를 크게 포함하고 늦게 반응한다.

4. **동일한 이동평균을 매 환경 step에 반복 기록한다.** CleanRL-1은 약 49.9만 step history row에 비해 실제 `episode_length` 변경은 671번뿐이다. 이 방식은 W&B 곡선을 긴 plateau와 큰 계단 형태로 보이게 하고 원시 로그 크기도 불필요하게 키운다.

5. **rollout 안의 종료 횟수에 따라 `step_reward_mean`도 순간 하락할 수 있다.** 종료 reward `-10`이 포함된 rollout은 현재 정책이 전반적으로 붕괴하지 않았더라도 낮은 값이 나올 수 있다.

### 3.2 그러나 실제 정책 회귀도 존재한다

CleanRL-1의 episode 이동평균만 내려간 것이 아니다.

- `episode_length`는 1,322,264 step에서 10,800에 도달한 뒤 1,691,088 step에서 5,841.97까지 45.9% 하락했다.
- `episode_reward`는 최고 2,523.01에서 1,280.01까지 49.3% 하락했다.
- 더 즉시적인 50-update `step_reward_mean` 이동평균도 최고 0.23976에서 최종 0.19191까지 20.0% 하락했다.
- 마지막 25%의 평균 KL은 0.00876, clip fraction은 0.10887이었으며 각각 최고 0.03093, 0.33579까지 상승했다. 고정 learning rate로 후기에도 정책 update가 계속 크게 일어났음을 보여준다.

즉 episode 곡선의 진폭 전체가 실제 붕괴는 아니지만, **최고 정책을 학습 후반까지 안정적으로 보존하지 못하는 문제는 실제로 있다.** `checkpoint_best.pt`를 사용하면 실용적 영향은 줄지만, `checkpoint_end.pt`를 신뢰하려면 개선할 가치가 있다.

### 3.3 원인 우선순위

| 우선순위 | 원인 | 판단 |
|---:|---|---|
| 1 | 장기 stochastic episode + 30-episode 이동평균의 가변 지연 | 표시 진동을 크게 증폭하는 주원인 |
| 2 | 고정 learning rate로 인한 후기 policy drift | 실제 후기 성능 회귀의 주요 후보 |
| 3 | 64개 sample의 작은 minibatch와 on-policy sample 분산 | update 간 변동을 키우는 후보 |
| 4 | 고정 entropy 보너스와 sampled training action | 실패 episode가 계속 발생하는 자연스러운 원인 |
| 5 | KL 제한 부재 | 큰 update를 막는 안전장치가 없지만 단독 주원인은 아님 |
| 6 | critic 불안정 | 일부 loss spike는 있으나 explained variance 중앙값이 높아 1차 원인은 아님 |

## 4. 해결해야 하는 문제인가?

판단은 다음처럼 나뉜다.

- **학습 곡선의 개별 spike/episode 간 편차:** 자연스러운 부분이 크므로 제거 자체를 목표로 하면 안 된다.
- **현재 episode 이동평균의 해석 어려움:** 해결해야 할 모니터링 문제다. 실제 정책 품질과 지연된 stochastic train 결과를 구분할 수 없다.
- **최고점 이후 rollout 보상과 episode 성능의 장기 하락:** 완화할 가치가 있는 실제 최적화 문제다.
- **배포 관점:** deterministic 평가 기준 `checkpoint_best.pt`를 선택한다면 긴급도는 낮다. `checkpoint_end.pt`를 그대로 사용할 계획이라면 해결 우선순위가 높다.

## 5. CleanRL-2 config 변경

요청대로 학습 코드나 환경 코드는 수정하지 않고 루트 `config.yaml`만 변경했다.

```yaml
env_config:
  exp_name: CleanRL-2          # CleanRL-1 -> CleanRL-2

training_config:
  anneal_lr: true              # false -> true
  ent_coef: 0.01               # 0.02 -> 0.01
  target_kl: 0.015             # null -> 0.015
```

변경 의도는 다음과 같다.

- `anneal_lr: true`: 후기로 갈수록 update 크기를 줄여 이미 학습한 좋은 정책을 보존한다.
- `ent_coef: 0.01`: 지속적인 탐험 압력을 낮춰 불필요한 후기 action randomness를 완화한다.
- `target_kl: 0.015`: 한 rollout을 여러 epoch 재사용하면서 policy가 과도하게 이동하는 경우 조기 중단한다.

비교 가능성을 위해 `seed`, `randomize_initial_state`, `total_timesteps`, `learning_rate`, `num_envs`, `num_steps`, `num_minibatches`, `update_epochs`, `gamma`, `gae_lambda` 등은 그대로 유지했다.

## 6. 테스트 실행 요약

- Run 이름: `CleanRL-2`
- Run ID: `49qod1v9`
- W&B 태그: `Codex`
- 상태: `finished`
- 실행 시간: 3,147초(약 52분 27초)
- effective timestep: 1,998,848
- 업로드 확인: `checkpoint/checkpoint_best.pt`, `checkpoint/checkpoint_end.pt`, config, summary, history
- 테스트 횟수: 1회
- 학습 종료 후 수동 rendering 창은 테스트 범위에서 제외하고 동일한 `Agent.learn()` 경로만 실행

분석 및 테스트 과정에서 수행한 작업은 다음과 같다.

1. CleanRL-1 W&B config와 로컬 동기화 config 대조
2. PPO update, reward, termination, Monitor 및 W&B logger 코드 읽기
3. CleanRL-1 전체 raw history 499,712 row 재집계
4. 원인 가설에 맞춰 `config.yaml`의 세 항목과 Run 이름 변경
5. Pydantic config validation 및 batch/effective timestep 확인
6. `Codex` 태그로 CleanRL-2 1회 전체 학습
7. 실행 중 주요 구간의 episode, rollout, KL, clip fraction 추적
8. 완료 후 W&B 상태, 태그, summary, 두 checkpoint 업로드 확인
9. 두 Run 전체 raw history를 같은 기준으로 재집계

## 7. 테스트 결과 비교

### 7.1 핵심 성능 및 진동 지표

| 지표 | CleanRL-1 | CleanRL-2 | 해석 |
|---|---:|---:|---|
| 최고 `episode_length` | 10,800.00 | 10,079.97 | -6.7%, 최고 성능 저하 |
| 최종 `episode_length` | 9,295.03 | 7,440.97 | -19.9%, 최종 장기 성능 저하 |
| 최고점 이후 최대 length 하락률 | 45.9% | 41.1% | 4.8%p 완화 |
| 마지막 25% length 평균 | 8,176.67 | 8,396.44 | +2.7% |
| 마지막 25% length 표준편차 | 2,141.64 | 1,511.45 | -29.4%, W&B 후기 곡선 안정화 |
| 최고 `episode_reward` | 2,523.01 | 2,158.62 | -14.4% |
| 최종 `episode_reward` | 1,991.45 | 1,662.32 | -16.5% |
| 최고점 이후 최대 reward 하락률 | 49.3% | 40.2% | 9.0%p 완화 |
| 마지막 25% reward 평균 | 1,844.59 | 1,804.29 | -2.2% |
| 마지막 25% reward 표준편차 | 519.85 | 305.91 | -41.2% |

### 7.2 즉시성이 높은 rollout 및 update 지표

| 지표 | CleanRL-1 | CleanRL-2 | 해석 |
|---|---:|---:|---|
| 마지막 25% `step_reward_mean` | 0.21817 ± 0.01831 | 0.21829 ± 0.01572 | 평균 동일, 표준편차 -14.1% |
| 마지막 100 update reward | 0.20824 ± 0.02128 | 0.22217 ± 0.01614 | 평균 +6.7%, 표준편차 -24.1% |
| 50-update reward 최고→최종 유지율 | 80.0% | 98.5% | 후기 rollout 보존 개선 |
| 마지막 25% 평균 KL | 0.00876 | 0.00322 | -63.3% |
| 전체 최대 평균 KL | 0.03543 | 0.01525 | 큰 policy update 억제 |
| 마지막 25% 평균 clip fraction | 0.10887 | 0.02460 | -77.4% |
| 마지막 25% 평균 entropy | 1.32465 | 1.46392 | 오히려 +10.5% |

마지막 entropy가 높아진 점은 중요하다. `ent_coef`를 낮추면 entropy 보너스 압력은 약해지지만, 실제 entropy가 반드시 낮아지는 것은 아니다. 학습률 감소 및 다른 변경과 상호작용하면서 CleanRL-2 정책은 오히려 더 불확실한 상태로 남았다. 따라서 이번 결과로 `ent_coef: 0.01`이 action randomness를 줄였다고 볼 수 없다.

### 7.3 종합 판정

CleanRL-2는 update 크기와 후기 rollout 변동을 확실히 줄였다. 그러나 episode 곡선의 진동을 제거하지 못했고, 최고 및 최종 episode 성능을 희생했다. 특히 30%, 50%, 63% 부근에서 큰 성능 하락이 여전히 관찰됐다. 후기 75–100% 평균은 안정됐지만 최종 episode 이동평균은 지연된 실패 episode의 영향으로 다시 낮아졌다.

따라서 결과는 다음과 같이 판정한다.

- **후기 optimizer 안정성:** 개선
- **후기 rollout reward 안정성:** 개선
- **episode-level 진동 제거:** 실패
- **최고 성능:** 악화
- **최종 episode 성능:** 악화
- **현재 세 변경의 기본값 채택:** 보류

## 8. 권장 후속 조치

### 8.1 config-only 후속 실험

다음 실험은 인과를 분리하기 위해 한 번에 한 축만 바꾸는 것이 좋다.

1. **최우선: `anneal_lr` 단독 ablation**

   - `anneal_lr: true`
   - `ent_coef: 0.02`로 복원
   - `target_kl: null`로 복원

   이번 Run의 후기 KL/clip fraction 감소와 50-update reward 보존은 학습률 감소가 만든 효과일 가능성이 높다. 반면 낮춘 entropy 계수는 실제 entropy를 낮추지 않았고, KL 제한은 코드상 epoch 마지막 minibatch 하나로 조기 중단을 판단하므로 효과를 별도로 검증해야 한다.

2. **minibatch 분산 단독 실험**

   - `num_minibatches: 16`으로 변경해 minibatch를 64 → 128로 확대
   - 나머지는 CleanRL-1 값 유지

   더 큰 minibatch는 update gradient 분산을 줄일 수 있다. 다만 optimizer step 수도 바뀌므로 학습 속도와 최종 성능을 함께 봐야 한다.

3. **KL 제한을 쓸 경우 단독 실험**

   - CleanRL-1 설정에서 `target_kl: 0.015` 또는 더 완만한 `0.02`만 추가

   현재 구현은 epoch 전체 평균이 아니라 마지막 minibatch의 KL로 중단한다. 따라서 threshold 민감도가 크며 단독 검증이 필요하다.

각 실험은 최소 3개 seed를 권장한다. 이번처럼 한 seed, 한 Run만으로는 stochastic PPO의 차이를 통계적으로 확정할 수 없다.

### 8.2 코드 변경이 필요한 권장 사항

요청 범위 때문에 이번에는 수정하지 않았다.

1. **고정 seed bank의 deterministic 평가 추가**

   일정 update마다 학습과 분리된 환경에서 `deterministic=True`로 20–50 episode를 평가하고 `eval/episode_length`, `eval/episode_reward`, `eval/success_rate`를 기록한다. 이것이 실제 정책 회귀 여부를 판단하는 가장 중요한 개선이다.

2. **episode logger 개선**

   episode가 끝났을 때만 raw episode 값을 기록하고, 별도 EMA 또는 일정 timestep 창 평균을 기록한다. 현재처럼 동일한 30-episode 평균을 매 step 기록하지 않는 것이 좋다.

3. **성공률 및 reward component 로깅**

   `truncated` 비율(10,800 step 생존 성공률), terminal 비율, `live/x/y/angular` reward component를 분리해 기록한다. 현재 reward와 length의 상관이 0.998이라 두 지표만으로 제어 품질을 구분하기 어렵다.

4. **현재 learning rate와 실제 실행 epoch 수 기록**

   `train/learning_rate`, `train/update_epochs_executed`를 남겨 annealing과 `target_kl` 조기 중단의 실제 작동을 검증한다.

5. **학습률/entropy schedule을 독립적으로 설정 가능하게 확장**

   현재 learning rate annealing은 0에 가까워지는 단일 선형 schedule뿐이다. minimum LR을 둔 schedule과 entropy annealing을 config에서 별도로 제어할 수 있으면 최고 성능과 후기 보존의 균형을 찾기 쉽다.

6. **best checkpoint 선정 기준을 deterministic eval로 변경**

   현재 best checkpoint는 지연이 큰 stochastic train `episode_length` 이동평균 기준이다. 고정 평가 seed의 평균 또는 하위 분위수 기준이 더 신뢰할 수 있다.

## 9. 한계

- CleanRL-2는 단 한 번 실행했으며 `torch_deterministic: false`이다.
- 세 config 값을 동시에 바꿨으므로 각각의 인과 효과를 분리할 수 없다.
- train episode는 학습 도중 계속 변하는 stochastic 정책으로 생성되며 별도 evaluation이 아니다.
- `episode_*`는 raw episode 값이 아니라 최근 30 episode 평균이다.
- 환경의 진짜 목표가 단순 생존인지, 중앙/자세 유지 품질까지 포함하는지 별도 성공 정의가 없다.

이 한계를 고려해 이번 테스트는 “학습률 감소와 update 제한으로 후기 rollout 안정성을 높일 수 있다”는 근거는 제공하지만, 최종 권장 hyperparameter를 확정하는 실험은 아니다.

