#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>

#include <curl/curl.h>
#include <json-c/json.h>

typedef struct {
  uint32_t markOfMastery;
  uint32_t markOfMasteryI;
  uint32_t markOfMasteryII;
  uint32_t markOfMasteryIII;
} Achievements;

typedef struct {
  uint32_t damageDealt;
  uint32_t shots;
  uint32_t hits;
  uint32_t wins;
  uint32_t losses;
  uint32_t battles;
  uint32_t survivedBattles;
  uint32_t frags;
  uint32_t spots;
  Achievements achievements;
} Stats;

const char* accountId = "126219745";
const char* appId = "74198832ec124e1cfe22490f35a7085f";

const char* timeUrl = "https://api.wotblitz.ru/wotb/account/info/?application_id=%s&account_id=%s&fields=last_battle_time";
const char* dataUrl = "https://api.wotblitz.ru/wotb/account/info/?application_id=%s&account_id=%s&fields=statistics.all.battles%%2Cstatistics.all.damage_dealt%%2Cstatistics.all.wins%%2Cstatistics.all.survived_battles%%2Cstatistics.all.hits%%2Cstatistics.all.shots%%2Cstatistics.all.losses%%2Cstatistics.all.frags%%2Cstatistics.all.spotted";
const char* achievementUrl = "https://api.wotblitz.ru/wotb/account/achievements/?application_id=%s&account_id=%s&fields=achievements";

time_t lastBattleTime, newTime;

Stats initialStats, lastStats, battleStats, sessionStats, currentStats;

size_t timeParse(void* buffer, size_t size, size_t nmemb, time_t *pTime);
size_t dataParse(void* buffer, size_t size, size_t nmemb, Stats *pStats);
size_t achievementParse(void* buffer, size_t size, size_t nmemb, Achievements *pAchievements);

void calculateStatsDifference(Stats *pMinuendStats, Stats *pSubtrahendStats, Stats *pResultStats);

int main() {
  curl_global_init(CURL_GLOBAL_ALL);
  char *url = (char*)malloc(sizeof(char) * 999); 

  CURL *timeHandle = curl_easy_init();
  sprintf(url, timeUrl, appId, accountId);
  curl_easy_setopt(timeHandle, CURLOPT_URL, url);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEFUNCTION, timeParse);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEDATA, &lastBattleTime);
  curl_easy_perform(timeHandle);
  curl_easy_setopt(timeHandle, CURLOPT_WRITEDATA, &newTime);

  CURL *dataHandle = curl_easy_init();
  sprintf(url, dataUrl, appId, accountId);
  curl_easy_setopt(dataHandle, CURLOPT_URL, url);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEFUNCTION, dataParse);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEDATA, &initialStats);
  curl_easy_perform(dataHandle);
  curl_easy_setopt(dataHandle, CURLOPT_WRITEDATA, &currentStats);

  CURL *achievementHandle = curl_easy_init();
  sprintf(url, achievementUrl, appId, accountId);
  curl_easy_setopt(achievementHandle, CURLOPT_URL, url);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEFUNCTION, achievementParse);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEDATA, &initialStats.achievements);
  curl_easy_perform(achievementHandle);
  curl_easy_setopt(achievementHandle, CURLOPT_WRITEDATA, &currentStats.achievements);

  free(url);
  lastStats = initialStats;

  printf("\x1B[2J\x1B[H");
  printf(" num | damage | hitrate | winrate | survival | frags | spots | M\n");
  for (;;) {
    curl_easy_perform(timeHandle);
    if (newTime != lastBattleTime) {
      lastBattleTime = newTime;
      curl_easy_perform(dataHandle);
      curl_easy_perform(achievementHandle);
      calculateStatsDifference(&currentStats, &lastStats, &battleStats);
      calculateStatsDifference(&currentStats, &initialStats, &sessionStats);
      printf("\x1B[2K\r");
      printf("%4i | %6i | %6.2f%% | %7s | %8s | %5d | %5d | %c\n", 
             sessionStats.battles,
             battleStats.damageDealt,
             (float)battleStats.hits / battleStats.shots * 100,
             battleStats.wins ? "win" : battleStats.losses ? "lose" : "draw",
             battleStats.survivedBattles ? "survived" : "died",
             battleStats.frags,
             battleStats.spots,
             battleStats.achievements.markOfMastery ? 'M' :
             battleStats.achievements.markOfMasteryI ? '1' :
             battleStats.achievements.markOfMasteryII ? '2' :
             battleStats.achievements.markOfMasteryIII ? '3' : '-');
      printf(" avg |%7.1f | %6.2f%% | %6.2f%% | %7.2f%% | %5.2f | %5.2f | %c",
             (float)sessionStats.damageDealt / sessionStats.battles,
             (float)sessionStats.hits / sessionStats.shots * 100,
             (float)sessionStats.wins / sessionStats.battles * 100,
             (float)sessionStats.survivedBattles / sessionStats.battles * 100,
             (float)sessionStats.frags / sessionStats.battles,
             (float)sessionStats.spots / sessionStats.battles,
             sessionStats.achievements.markOfMastery ? 'M' :
             sessionStats.achievements.markOfMasteryI ? '1' :
             sessionStats.achievements.markOfMasteryII ? '2' :
             sessionStats.achievements.markOfMasteryIII ? '3' : '-');
      fflush(stdout);
      lastStats = currentStats;
    }
    sleep(1);
  }
  curl_global_cleanup();
  return 0;
}

size_t timeParse(void *buffer, size_t size, size_t nmemb, time_t *pTime) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *last_battle_time;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "last_battle_time", &last_battle_time);
  *pTime = json_object_get_int(last_battle_time);
  return size * nmemb;
}

size_t dataParse(void *buffer, size_t size, size_t nmemb, Stats *pStats) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *statistics, *all;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "statistics", &statistics);
  json_object_object_get_ex(statistics, "all", &all);
  json_object *damage_dealt, *shots, *hits, *battles, *survived_battles, *wins, *losses, *frags, *spotted;
  json_object_object_get_ex(all, "damage_dealt", &damage_dealt);
  json_object_object_get_ex(all, "shots", &shots);
  json_object_object_get_ex(all, "hits", &hits);
  json_object_object_get_ex(all, "wins", &wins);
  json_object_object_get_ex(all, "battles", &battles);
  json_object_object_get_ex(all, "survived_battles", &survived_battles);
  json_object_object_get_ex(all, "losses", &losses);
  json_object_object_get_ex(all, "frags", &frags);
  json_object_object_get_ex(all, "spotted", &spotted);
  pStats->damageDealt = json_object_get_int(damage_dealt);
  pStats->hits = json_object_get_int(hits);
  pStats->shots = json_object_get_int(shots);
  pStats->wins = json_object_get_int(wins);
  pStats->losses = json_object_get_int(losses);
  pStats->battles = json_object_get_int(battles);
  pStats->survivedBattles = json_object_get_int(survived_battles);
  pStats->frags = json_object_get_int(frags);
  pStats->spots = json_object_get_int(spotted);
  return size * nmemb;
}

size_t achievementParse(void *buffer, size_t size, size_t nmemb, Achievements *pAchievements) {
  json_object *obj = json_tokener_parse(buffer);
  json_object *data, *me, *achievements;
  json_object_object_get_ex(obj, "data", &data);
  json_object_object_get_ex(data, accountId, &me);
  json_object_object_get_ex(me, "achievements", &achievements);
  json_object *markOfMastery, *markOfMasteryI, *markOfMasteryII, *markOfMasteryIII;
  json_object_object_get_ex(achievements, "markOfMastery", &markOfMastery);
  json_object_object_get_ex(achievements, "markOfMasteryI", &markOfMasteryI);
  json_object_object_get_ex(achievements, "markOfMasteryII", &markOfMasteryII);
  json_object_object_get_ex(achievements, "markOfMasteryIII", &markOfMasteryIII);
  pAchievements->markOfMastery = json_object_get_int(markOfMastery);
  pAchievements->markOfMasteryI = json_object_get_int(markOfMasteryI);
  pAchievements->markOfMasteryII = json_object_get_int(markOfMasteryII);
  pAchievements->markOfMasteryIII = json_object_get_int(markOfMasteryIII);
  return size * nmemb;
}

void calculateStatsDifference(Stats *pMinuendStats, Stats *pSubtrahendStats, Stats *pResultStats) {
  pResultStats->damageDealt = pMinuendStats->damageDealt - pSubtrahendStats->damageDealt;
  pResultStats->hits = pMinuendStats->hits - pSubtrahendStats->hits;
  pResultStats->shots = pMinuendStats->shots - pSubtrahendStats->shots;
  pResultStats->wins = pMinuendStats->wins - pSubtrahendStats->wins;
  pResultStats->losses = pMinuendStats->losses - pSubtrahendStats->losses;
  pResultStats->battles = pMinuendStats->battles - pSubtrahendStats->battles;
  pResultStats->survivedBattles = pMinuendStats->survivedBattles - pSubtrahendStats->survivedBattles;
  pResultStats->frags = pMinuendStats->frags - pSubtrahendStats->frags;
  pResultStats->spots = pMinuendStats->spots - pSubtrahendStats->spots;
  pResultStats->achievements.markOfMastery = pMinuendStats->achievements.markOfMastery - pSubtrahendStats->achievements.markOfMastery;
  pResultStats->achievements.markOfMasteryI = pMinuendStats->achievements.markOfMasteryI - pSubtrahendStats->achievements.markOfMasteryI;
  pResultStats->achievements.markOfMasteryII = pMinuendStats->achievements.markOfMasteryII - pSubtrahendStats->achievements.markOfMasteryII;
  pResultStats->achievements.markOfMasteryIII = pMinuendStats->achievements.markOfMasteryIII - pSubtrahendStats->achievements.markOfMasteryIII;
}
