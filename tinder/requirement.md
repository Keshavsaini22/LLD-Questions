User can swipe left/right to a profile
User can setup his/her own profile
User can set his/her perferences
Once there is a match they can chat in chatroom
User can see all the profiles near their location(Nearby can be based on different strategy)
User should get notified when there is a match or receive a new message
User matching should be based on several factors and scores (like interst match,location match etc etc) -> recommendation



In c++ code there if interviewer says that we need to recommend the best profiles to the user than i need recommend strategy otherwise there is no need
So read the chat for tinder in chatgpt.

The responsibilities are very clean:
DiscoveryService → Finds eligible candidate profiles.
ProfileRankingStrategy → Decides the order of those candidates.
MatchService → Handles swipes and mutual matches.
ChatService → Handles messaging.
NotificationService → Delivers notifications.


CandidateGenerationStrategy-optional
Netflix does
100 Million Movies
↓
Candidate Generation
↓
500 Movies
↓
Ranking Model
↓
Top 20 Movies
So discovery of candidate should have 2 logics first is to get valid candidates like here we have near by and second is sort them to get best upper--recommendation
                     DiscoveryService
                           |
        ------------------------------------
        |                                  |
        V                                  V
Candidate Generation            Profile Ranking
(Filter Eligible Users)       (Score & Sort Users)
        |                                  |
        ----------- Candidates -------------
                           |
                           V
                    Return Top K